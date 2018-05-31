ifeq ($(HOME),/home/yuandong)
  GCC=$(HOME)/gcc-4.9.3/bin/gcc -I /home/yuandong/boost_1_61_0 
  ANACONDA=/home/yuandong/anaconda3/bin
else
  GCC=g++
  ANACONDA=$(CONDA_PREFIX)/bin
endif


PYTHON_OPTIONS=$(shell ${ANACONDA}/python3-config --cflags --libs)
PYTHON_LIB=$(shell ${ANACONDA}/python3-config --configdir)
INCLUDE = -I../ -I../vendor -I../vendor/pybind11/include
LIBRARIES = -lstdc++ -lm -ltbb

all:
	${GCC} -Wall --std=c++11 -pthread test_elf.cpp ${INCLUDE} -o test_elf.bin ${LIBRARIES}

test_elf.so:
	${GCC} -fPIC -shared -Wall --std=c++11 -pthread test_elf_python.cc ${INCLUDE}  ${PYTHON_OPTIONS} -L${PYTHON_LIB} -o test_elf.so ${LIBRARIES}

debug:
	${GCC} -Wall -g --std=c++11 -fsanitize=address,undefined test_elf.cpp ${INCLUDE} -o test_elf.bin ${LIBRARIES}

test_elf_debug.so:
	${GCC} -Wall -g -fPIC -shared --std=c++11 -pthread -fsanitize=address,undefined test_elf_python.cc ${INCLUDE} ${PYTHON_OPTIONS} -L${PYTHON_LIB} -o test_elf.so ${LIBRARIES}

clean:
	rm *.so *.bin

test.so:
	${GCC} -fPIC -shared -Wall --std=c++11 -pthread ./test/test.cc ${INCLUDE} ${PYTHON_OPTIONS} -L${PYTHON_LIB} -o ./test/test.so ${LIBRARIES}

