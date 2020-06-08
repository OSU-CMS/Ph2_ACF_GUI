from distutils.core import setup, Extension

module1 = Extension('example',
                    sources = ['example.C'])

setup(name = 'example',
    version = '1.0',
    description = 'example package',
    ext_modules = [module1])