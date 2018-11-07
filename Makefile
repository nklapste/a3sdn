# ------------------------------------------------------------
# Nathan Klapstein - CMPUT 379 A3
#
# It is preferred to use Cmake over Makefiles. It is not
# the stone age anymore.
#
# Usage: make // compile programs
#        make tar // create a 'tar.gz' archive of 'allFiles'
#        make clean // remove unneeded files
# ------------------------------------------------------------

target = submit
allFiles = README.md LICENSE Makefile gate.h gate.cpp connection.cpp connection.h controller.cpp controller.h flow.h packet.cpp packet.h \
            switch.cpp switch.h a3sdn.cpp tf1.txt tf2.txt tf3.txt trafficfile.cpp trafficfile.h

compile:
	g++-7 -std=c++11 -Wall trafficfile.cpp trafficfile.h gate.cpp gate.h connection.cpp connection.h controller.cpp controller.h flow.h packet.cpp packet.h switch.cpp switch.h a3sdn.cpp -o a3sdn

tar:
	touch $(target).tar.gz
	mv $(target).tar.gz  x$(target).tar.gz
	tar -cvf $(target).tar $(allFiles)
	gzip $(target).tar

clean:
	rm -f *~ out.* *.o
