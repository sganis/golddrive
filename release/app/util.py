# util module for running commands and load config
import subprocess

def run(cmd):
	#print(f'Running command:\n{cmd}')
	proc = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	ret = 0
	try:
		out, err = proc.communicate(timeout=30)
	except subprocess.TimeoutExpired:
		proc.kill()
		out, err = proc.communicate()
	ret = proc.returncode
	if err:
		err = err.decode('ascii').strip()
		print(f'error: {err}')
	out = out.decode('ascii').strip()
	# print(f'output: {out}')
	# print(f'return: {ret}')
	return out, err, ret


if __name__ == '__main__':
	run('tasklist')