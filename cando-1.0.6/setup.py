from setuptools import setup

setup(
    name = 'cando',
    py_modules=['cando'],
    version='1.0.6',
    description='Python USB-CAN(Cando & Cando_pro) access module',
    author='Codenocold',
    author_email='codenocold@gmail.com',
    license='BSD',
    url='https://shop104034926.taobao.com/?spm=2013.1.1000126.2.18816badW0zk9O',
    keywords = ['Cando', 'Cando_pro', 'USB-CAN'],
    install_requires = [
      'PyUSB==1.0.2',    # Required to access USB devices from Python through libusb
    ],
)
