# ------------------------------------------------------------
# Nathan Klapstein - CMPUT 379 A2
#
# It is preferred to use Cmake over Makefiles. It is not
# the stone age anymore.
#
# Usage: make // compile programs
#        make tar // create a 'tar.gz' archive of 'allFiles'
#        make clean // remove unneeded files
# ------------------------------------------------------------

target = submit
allFiles = Makefile a2sdn.cpp

compile:
	g++ -std=c++11 -Wall a2sdn.cpp -o a2sdn

tar:
	touch $(target).tar.gz
	mv $(target).tar.gz  x$(target).tar.gz
	tar -cvf $(target).tar $(allFiles)
	gzip $(target).tar

clean:
	rm -f *~ out.* *.o
