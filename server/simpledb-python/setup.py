from distutils.core import setup, Extension

module1 = Extension('simpledb', sources = ['pysimpledb.c'], libraries= ['simpledb'])

setup (name = 'PySimpleDb',
        version = '1.0',
        description = 'simpledb bindings for Python',
        ext_modules = [module1])
