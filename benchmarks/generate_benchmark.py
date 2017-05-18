#!/usr/bin/env python3

import argparse
import em
import os
import os.path
import random
import string
import sys

parser = argparse.ArgumentParser("generate benchmarks for reflopt")

parser.add_argument('N', type=int, nargs=1, help='The number of arguments to generate')
parser.add_argument('M', type=int, nargs=1, help='The maximum length of argument names.')
parser.add_argument('--in_filename', dest='in_filename', type=str, nargs=1, help='Filename to save output to.')
parser.add_argument('--out_filename', dest='out_filename', type=str, nargs=1, help='Filename to save output to.')

args = parser.parse_args(sys.argv[1:])

N = args.N[0]
M = args.M[0]
in_filename = args.in_filename[0]
out_filename = args.out_filename[0]

arguments = set()

alpha = string.ascii_lowercase
for i in range(N):
    arguments.add(''.join([alpha[(i + j) % len(alpha)] for j in range(M)]))

out_dir = os.path.dirname(out_filename)
if not os.path.exists(out_dir):
    os.makedirs(out_dir)

interpreter = em.Interpreter(output=open(out_filename, 'w+'))

interpreter.globals['arguments'] = arguments

interpreter.file(open(in_filename))
interpreter.shutdown()
