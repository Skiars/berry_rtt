import os
from building import * 

# get current dir path
cwd = GetCurrentDir()

# init src and inc vars
src = []
inc = []

# add berry common include
inc = inc + [cwd + "/port_rtt"] + [cwd + "/berry/src"]

# add berry basic code
src    += Glob('port_rtt/*.c')
src    += Glob('berry/src/*.c')

objs = DefineGroup('berry', src, depend = ['PKG_USING_BERRY'], CPPPATH = inc)

if not os.path.exists('generate'):
    os.mkdir('generate')

os.system('map_build -o generate -i port_rtt berry/src -c port_rtt/berry_conf.h')

Return('objs')
