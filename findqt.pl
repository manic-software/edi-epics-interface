open PRO, ">tmp.pro";
select PRO;

print "QT+=sql\n";
print "QT+=network\n";

close PRO;

open QT, ">makefile.qt";
select QT;

@lines=`/usr/lib/qt4/bin/qmake -makefile -o - tmp.pro`;

foreach $line (@lines) {
    chomp $line;
    if ($line!~/^\s*#/) {
	if (my ($param, $value)=$line=~/^\s*(\S*)\s*=\s*(.*)\s*/) {
	    $value=~s/(\.\.\/)+/C:\\/g;
            #remove duplicate entries
	    $value=~s/(\S+)(\s+\1)/\1/g;
	    if ($param=~"CXXFLAGS") {
		print "QT_CXXFLAGS:=$value\n";
	    }
	    elsif ($param=~"INCPATH") {
		$value=~s/-I\.//g;
		$value=~s/-I\'\.\'//g;
		print "QT_INCPATH:=$value\n";
		$value=~s/-I/-isystem /g;
		print "QT_INCDEP:=$value\n";
	    }
	    elsif ($param=~"LIBS") {
		print "QT_LIBS:=$value\n";
	    }
	    elsif ($param=~"LFLAGS") {
		print "QT_LFLAGS:=$value\n";
	    }
	}
    }
}
close QT;

`rm -f tmp.pro`;
`dos2unix makefile.qt`;
