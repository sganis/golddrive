# util module for running commands and load config
import subprocess

class ReturnBox():
	def __init__(self, out='', err=''):
		self.output = out
		self.error = err
		self.drive_state = 'DISCONNECTED'

def run(cmd, output=False, timeout=10):
	# print(f'CMD:\n{cmd}')
	shell=False
	if '"' in cmd or '%' in cmd or '>' in cmd:
		shell=True
	r = subprocess.run(cmd, timeout, shell=shell, capture_output=output, text=True)
	# print(f'{r}')
	if output:
		if r.stderr.strip():
			print(f'ERROR:\n{r}')
		return r.stdout.strip(), r.stderr.strip(), r.returncode
	return ('','',r.returncode)
	# proc = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	# ret = 0
	# try:
	# 	out, err = proc.communicate(timeout=30)
	# except subprocess.TimeoutExpired:
	# 	proc.kill()
	# 	out, err = proc.communicate()
	# ret = proc.returncode
	# if err:
	# 	err = err.decode('ascii').strip()
	# 	print(f'error: {err}')
	# out = out.decode('ascii').strip()
	# # print(f'output: {out}')
	# # print(f'return: {ret}')
	# return out, err, ret


if __name__ == '__main__':
	run('tasklist')	
	run(fr'more "%USERPROFILE%\.ssh\id_rsa"')	
	run(fr'echo "hello world"')