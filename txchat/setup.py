from setuptools import setup

setup(
    name='txchat',
    version='1.0.0',
    description='async demo for @PolyGlotAB',
    author='meejah',
    author_email='meejah@meejah.ca',
    url='https://meejah.ca',
    platforms=('Any'),
    install_requires=[
        'Twisted==15.0.0',
        'msgpack-python==0.4.6',
        'click==3.3',
    ],
    extras_require={
        'dev': [
            'pytest==2.6.4',
            'pytest-cov==1.8.1',
            'pep8==1.5.7',
            'flake8==2.4.0',
            'mock==1.0.1',
        ],
    },
    packages=['txchat'],
    entry_points={
        'console_scripts': [
            'txchat = txchat.cli:chat'
        ]
    },
    classifiers=[
        "License :: OSI Approved :: MIT License",
        "Development Status :: 3 - Alpha",
        "Environment :: Console",
        "Framework :: Twisted",
        "Intended Audience :: Developers",
        "Operating System :: OS Independent",
        "Programming Language :: Python",
    ],
    keywords='asyncio twisted coroutine',
)
