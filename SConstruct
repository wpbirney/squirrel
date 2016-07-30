#!/bin/python

debug = ARGUMENTS.get("debug", 0)

sqsrcs = Glob("squirrel/*.c*")
sqstdsrcs = Glob("sqstdlib/*.c*")

env = Environment(CPPPATH=['include'], CPPFLAGS=[], LINKFLAGS=['-s'])

if debug == 0:
	env.Append(CPPFLAGS=['-O3'])
else:
	env.Append(CPPFLAGS=['-g'])

lib = env.Library("sq", source=[sqsrcs, sqstdsrcs])

env.Install('/usr/local/lib', lib)
env.Install('/usr/local/include', Glob("include/*.h*"))

env.Alias('install', ['/usr/local/lib', '/usr/local/include'])
