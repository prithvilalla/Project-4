# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

# def options(opt):
#     pass

# def configure(conf):
#     conf.check_nonfatal(header_name='stdint.h', define_name='HAVE_STDINT_H')

def build(bld):
    module = bld.create_ns3_module('worm', ['core', 'internet', 'applications', 'network'])
    module.source = [
        'model/worm.cc',
#        'helper/worm-helper.cc',
        ]

#    module_test = bld.create_ns3_module_test_library('worm')
#    module_test.source = [
#        'test/worm-test-suite.cc',
#        ]

    headers = bld(features='ns3header')
    headers.module = 'worm'
    headers.source = [
        'model/worm.h',
#        'helper/worm-helper.h',
        ]

#    if bld.env.ENABLE_EXAMPLES:
#        bld.recurse('examples')

    # bld.ns3_python_bindings()
