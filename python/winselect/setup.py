#!/usr/bin/env python
# -*- coding: utf-8 -*-

from setuptools import setup, Extension

setup(
    name='winselect',
    version='1.0',
    ext_modules=[
        Extension(
            name="winselect",
            sources=["selectmodule.c"],
            language="c",
            extra_link_args=["ws2_32.lib", "wepoll.lib"]
        ),
    ],
    zip_safe=True,
)
