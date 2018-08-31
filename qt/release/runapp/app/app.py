# app.py

def run():
	import remount
	remount.main()
	input("Press any key to exit this program...")

def test():
	import os
	folder = os.path.dirname(os.path.realpath(__file__))
	file = os.path.join(folder, 'app_test.log')
	with open(file, 'a') as w:
		w.write('test is ok')

if __name__ == '__main__':
	run()
