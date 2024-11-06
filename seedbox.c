#include <u.h>
#include <libc.h>

typedef struct Seed Seed;
struct Seed {
	int pid;
	char *torrent;
	int seen;
};
static Seed *seeds;
static int nseeds;
static char *ddir, *tdir;
static int tfd;

static void
spawn(Seed *s, char *t)
{
	int p;

	if(t != nil && (s->torrent = strdup(t)) == nil)
		sysfatal("strdup: %r");
	switch(p = fork()){
	case -1:
		sysfatal("fork: %r");
	case 0:
		if(chdir(ddir ? ddir : tdir) < 0)
			sysfatal("chdir: %r");
		execl("/bin/ip/torrent", "torrent", "-s", smprint("%s/%s", tdir, s->torrent), nil);
		sysfatal("exec: %r");
	default:
		s->pid = p;
		return;
	}
}

static void
kill(Seed *s)
{
	assert(s->pid != 0);
	postnote(PNPROC, s->pid, "kill");
	s->pid = 0;
	free(s->torrent);
	s->torrent = nil;
}

static void
scandir(void)
{
	Dir *d;
	char **new, *s;
	int i, j, n, o, p;

	for(j = 0; j < nseeds; j++)
		seeds[j].seen = 0;
	if(seek(tfd, 0, 0) != 0)
		sysfatal("seek: %r");
	n = dirreadall(tfd, &d);
	if(n < 0)
		sysfatal("dirreadall: %r");
	new = mallocz(sizeof(char*)*n, 1);
	if(new == nil)
		sysfatal("malloc: %r");
	o = 0;
	for(i = 0; i < n; i++){
		if((s = strstr(d[i].name, ".torrent")) == nil)
			continue;
		if(s[strlen(".torrent")] != '\0')
			continue;
		for(j = 0; j < nseeds; j++){
			if(strcmp(seeds[j].torrent, d[i].name) != 0)
				continue;
			if(seeds[j].pid == 0)
				spawn(seeds+j, nil);
			seeds[j].seen = 1;
			break;
		}
		if(j == nseeds)
			new[o++] = d[i].name;
	}
	for(j = 0; o > 0 && j < nseeds; j++){
		if(seeds[j].seen == 1)
			continue;
		kill(seeds+j);
		spawn(seeds+j, new[--o]);
		seeds[j].seen = 1;
	}
	if(o > 0){
		seeds = realloc(seeds, sizeof(Seed) * (nseeds + o));
		if(seeds == nil)
			sysfatal("realloc: %r");
		for(p = o, j = nseeds; j < nseeds + o; j++){
			seeds[j].seen = 1;
			spawn(seeds+j, new[--p]);
		}
		nseeds = nseeds + o;
	}
	for(j = 0; j < nseeds; j++)
		if(seeds[j].seen == 0)
			kill(seeds+j);
	free(new);
	free(d);
}

static int
catch(void*, char*)
{
	int j;

	for(j = 0; j < nseeds; j++)
		kill(seeds+j);
	return 0;
}

_Noreturn void
usage(void)
{
	fprint(2, "%s: tdir <ddir>\n", argv0);
	exits("usage");
}

void
main(int argc, char **argv)
{
	int sec;
	char *n, *r;
	char buf[128];

	sec = 30;
	ARGBEGIN{
	case 'w':
		n = EARGF(usage());
		sec = strtol(n, &r, 0);
		if(n == r)
			sysfatal("invalid number");
		break;
	default:
		usage();
	}ARGEND

	switch(argc){
	default:
		usage();
	case 2:
		ddir = argv[1];
	case 1:
		tfd = open(argv[0], OREAD);
		if(tfd < 0)
			sysfatal("open: %r");
		if(fd2path(tfd, buf, sizeof buf) != 0)
			sysfatal("fd2path: %r");
		tdir = strdup(buf);
		if(tdir == nil)
			sysfatal("tdir: %r");
	}

	switch(fork()){
	case -1:
		sysfatal("fork: %r");
	case 0:
		break;
	default:
		exits(0);
	}

	for(atnotify(catch, 1); ; sleep(sec * 1000))
		scandir();
}
