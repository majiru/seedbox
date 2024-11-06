</$objtype/mkfile

BIN=/$objtype/bin/ip
TARG=seedbox
OFILES=seedbox.$O

</sys/src/cmd/mkone

install:V:	$BIN/$TARG
	cp seedbox.1 /sys/man/1/seedbox
