import sys, subprocess

exe = sys.argv[1]
result = subprocess.run([exe] + sys.argv[1:])
sys.exit(result.returncode)
