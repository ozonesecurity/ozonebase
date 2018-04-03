Installation
*************

The examples below are for a typical Ubuntu/Debian system.

Installation of oZone libraries and examples
============================================

oZone is a portable solution with a very easy installation process.
This example assumes you want all the ozone libraries (including dependencies) to be installed at ~/ozoneroot.
This is a great way to isolate your install from other libraries you may already have installed.

There are two parts, a one time process and then building just the ozone library repeatedly (if you are making changes
to the examples or core code)

One time setup:

.. code-block:: bash

	# -------------------install dependencies------------------------

	sudo apt-get update
	sudo apt-get install git cmake nasm libjpeg-dev libssl-dev 
	sudo apt-get install libatlas-base-dev libfontconfig1-dev libv4l-dev

	# ---------------------clone codebase----------------------------

	git clone https://github.com/ozonesecurity/ozonebase
	cd ozonebase
	git submodule update --init --recursive

	# --------------- build & install --------------------------------
	export INSTALLDIR=~/ozoneroot/ # change this to whatever you want
        ./ozone-build.sh

.. note:: if you face compilation issues with ffmpeg not finding fontconfig or other package files, you need to search for libv4l2.pc, fontconfig.pc files and copy then to the lib/pkgconfig directory of your INSTALL_DIR path


Once the one time setup is done, you don't need to keep doing it (building external dependencies take a long time)
For subsequent changes, you can keep doing these steps:

.. code-block:: bash

	# ---- Optional: For ad-hoc in-source re-building----------------
	cd server
        cmake -DCMAKE_INSTALL_PREFIX=$INSTALLDIR -DOZ_EXAMPLES=ON -DCMAKE_INCLUDE_PATH=$INSTALLDIR/include
	make
	make install

	# ----- Optional: build nvrcli - a starter NVR example ----------
    	cd server
	edit src/examples/CMakeLists.txt and uncomment lines 14 and 27 (add_executable for nvrcli and target_link_libraries for nvrcli
 
	make

That's all!

Dlib optimizations
===================
If your processor supports AVX instructions, :code:`(cat /proc/cpuinfo | grep avx)` then add :code:`-mavx` in :code:`server/CMakeLists.txt` to :code:`CMAKE_C_FLAGS_RELEASE` and :code:`CMAKE_CXX_FLAGS_RELEASE` and rebuild. Note, please check before you add it, otherwise your code may core dump.  

Building Documentation
=======================
oZone documentation has two parts: 

* The API document that uses Doxygen
* The User Guide which is developed using Sphinx

API docs
---------
To build the APIs all you need is :code:`Doxygen` and simply run :code:`doxygen` inside :code:`ozonebase/server`.
This will generate HTML documents. oZone uses :code:`dot` to genarate API class and relationship graphs, so you should also install dot, which is typically part of the :code:`graphviz` package.

User docs
---------
You need sphinx and dependencies for generating your own user guide. The user guide source files are located in :code:`ozonebase/docs/server/guide` 

.. code-block:: bash

	# Install dependencies
	sudo apt-get install python-sphinx
	sudo apt-get install python-pip
	pip install sphinx_rtd_theme

And then all you need to do is :code:`make html` inside :code:`ozonebase/docs/server/guide` and it generates beautiful documents inside the :code:`build` directory.




Using oZone libraries in your own app
=========================================

Take a look at nvrcli's sample Makefile `here <https://github.com/ozonesecurity/ozonebase/blob/master/server/src/examples/nvrcli/Makefile.sample>`_ and modify it for your needs.


