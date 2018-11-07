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
allFiles = README.md LICENSE Makefile connection.cpp connection.h controller.cpp controller.h flow.h packet.cpp packet.h \
            switch.cpp switch.h a2sdn.cpp tf1.txt tf2.txt tf3.txt cmput379_assignment_2_report.pdf

compile:
	g++ -std=c++11 -Wall gate.cpp gate.h connection.cpp connection.h controller.cpp controller.h flow.h packet.cpp packet.h switch.cpp switch.h a3sdn.cpp -o a3sdn

tar:
	touch $(target).tar.gz
	mv $(target).tar.gz  x$(target).tar.gz
	tar -cvf $(target).tar $(allFiles)
	gzip $(target).tar

clean:
	rm -f *~ out.* *.o
