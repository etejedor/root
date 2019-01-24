import py, os, sys
from pytest import raises
from .support import setup_make


class TestREGRESSION:
    helpout = []

    def setup_class(cls):
        import cppyy

        def stringpager(text, cls=cls):
            cls.helpout.append(text)

        import pydoc
        pydoc.pager = stringpager

    def test01_kdcraw(self):
        """Doc strings for KDcrawIface (used to crash)."""

        import cppyy, pydoc

        # TODO: run a find for these paths
        qtpath = "/usr/include/qt5"
        kdcraw_h = "/usr/include/KF5/KDCRAW/kdcraw/kdcraw.h"
        if not os.path.isdir(qtpath) or not os.path.exists(kdcraw_h):
            import warnings
            warnings.warn("no KDE/Qt found, skipping test01_kdcraw")
            return

        # need to resolve qt_version_tag for the incremental compiler; since
        # it's not otherwise used, just make something up
        cppyy.cppdef("int qt_version_tag = 42;")
        cppyy.add_include_path(qtpath)
        cppyy.include(kdcraw_h)

        from cppyy.gbl import KDcrawIface

        self.__class__.helpout = []
        pydoc.doc(KDcrawIface.KDcraw)
        helptext  = ''.join(self.__class__.helpout)
        assert 'KDcraw' in helptext
        assert 'CPPInstance' in helptext

    def test02_dir(self):
        """For the same reasons as test01_kdcraw, this used to crash."""

        import cppyy, pydoc

        assert not '__abstractmethods__' in dir(cppyy.gbl.gInterpreter)
        assert '__class__' in dir(cppyy.gbl.gInterpreter)

        self.__class__.helpout = []
        pydoc.doc(cppyy.gbl.gInterpreter)
        helptext = ''.join(self.__class__.helpout)
        assert 'TInterpreter' in helptext
        assert 'CPPInstance' in helptext
        assert 'AddIncludePath' in helptext

        cppyy.cppdef("namespace cppyy_regression_test { void iii() {}; }")

        assert not 'iii' in cppyy.gbl.cppyy_regression_test.__dict__
        assert not '__abstractmethods__' in dir(cppyy.gbl.cppyy_regression_test)
        assert '__class__' in dir(cppyy.gbl.cppyy_regression_test)
        assert 'iii' in dir(cppyy.gbl.cppyy_regression_test)

        assert not 'iii' in cppyy.gbl.cppyy_regression_test.__dict__
        assert cppyy.gbl.cppyy_regression_test.iii
        assert 'iii' in cppyy.gbl.cppyy_regression_test.__dict__

        self.__class__.helpout = []
        pydoc.doc(cppyy.gbl.cppyy_regression_test)
        helptext = ''.join(self.__class__.helpout)
        # TODO: it's deeply silly that namespaces inherit from CPPInstance (in CPyCppyy)
        assert ('CPPInstance' in helptext or 'CPPNamespace' in helptext)

    def test03_pyfunc_doc(self):
        """Help on a generated pyfunc used to crash."""

        import cppyy, distutils, pydoc, sys

        cppyy.add_include_path(distutils.sysconfig_get_python_inc())
        if sys.hexversion < 0x3000000:
            cppyy.cppdef("#undef _POSIX_C_SOURCE")
            cppyy.cppdef("#undef _XOPEN_SOURCE")
        else:
            cppyy.cppdef("#undef slots")     # potentially pulled in by Qt/xapian.h

        cppyy.cppdef("""#include "Python.h"
           long py2long(PyObject* obj) { return PyLong_AsLong(obj); }""")

        pydoc.doc(cppyy.gbl.py2long)

        assert 1 == cppyy.gbl.py2long(1)

    def test04_avx(self):
        """Test usability of AVX by default."""

        import cppyy, subprocess

        has_avx = False
        try:
            for line in open('/proc/cpuinfo', 'r'):
                if 'avx' in line:
                    has_avx = True
                    break
        except Exception:
            try:
                cli_arg = subprocess.check_output(['sysctl', 'machdep.cpu.features'])
                has_avx = 'avx' in cli_arg.decode("utf-8").strip().lower()
            except Exception:
                pass

        if has_avx:
            assert cppyy.cppdef('int check_avx() { return (int) __AVX__; }')
            assert cppyy.gbl.check_avx()   # attribute error if compilation failed

    def test05_default_template_arguments(self):
        """Calling a templated method on a templated class with all defaults used to crash."""

        import cppyy

        cppyy.cppdef("""
        template<typename T>
        class AllDefault {
        public:
           AllDefault(int val) : m_t(val) {}
           template<int aap=1, int noot=2>
              int do_stuff() { return m_t+aap+noot; }

        public:
           T m_t;
        };""")

        a = cppyy.gbl.AllDefault[int](24)
        a.m_t = 21;
        assert a.do_stuff() == 24
