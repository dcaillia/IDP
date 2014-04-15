This is the readme for the IDP system.
IDP is a system for representing knowledge in the logic FO(.) and applying reasoning on it.
FO(.) is a logic consisting of first-order logic, inductive definitions, aggregates, partial functions and arithmetic.

Out of the box, the following inferences are supported:

   - grounding
   - modelexpansion
   - propagation
   - evaluating definitions
   - ...

# Installing and running the system
Required software packages:

   - C and C++ compiler, supporting most of the C++11 standard. Examples are GCC 4.4 or higher, clang 3.2 or visual studio 11.
   - Cmake build environment. 
   - Bison and flex packages or yacc and lex packages.
   - Pdflatex and doxygen for building the documentation. (On linux, search for the `latex2html` package)
   - (optional) Gecode for constraint programming support.

Assume idp is unpacked in `<idpdir>`, you want to build in `<builddir>` (cannot be the same as `<idpdir>`) and install in `<installdir>`.
Building and installing is then achieved by executing the following commands:
```
cd <builddir>
cmake <idpdir> -DCMAKE_INSTALL_PREFIX=<installdir> -DCMAKE_BUILD_TYPE="Release"
make -j 4
make check
make install
```
Alternatively, cmake-gui can be used as a graphical way to set cmake options.

# Adding constraint solving support by Gecode
Execute the following command in your <builddir> to enable support by Gecode:
```
cmake <idpdir> -DCMAKE_INSTALL_PREFIX=<installdir> -DCMAKE_BUILD_TYPE="Release" -DWITHGECODE=on
```

# Further information
For more information on using the system, see the documentation, which can be found in `docs/official/idp-manual.pdf`