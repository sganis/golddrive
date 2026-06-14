#!/usr/bin/env python3
from flask import Flask, request

app = Flask(__name__)

@app.route('/', defaults={'u_path': ''}, methods=['GET', 'POST'])
@app.route('/<path:u_path>', methods=['GET', 'POST'])
def home(u_path):
	if request.method == 'POST':
		m = request.form.get('message', '').replace('\r', '').replace('\n', '')
		t = request.form.get('try', '').replace('\r', '').replace('\n', '')
		print(f'{t}: {m}')
		return 'ok'
	else:		
		return 'Logger!'


if __name__ == '__main__':
	app.run(host='127.0.0.1', ssl_context='adhoc')
	# openssl req -x509 -newkey rsa:4096 -nodes -out cert.pem -keyout key.pem -days 365
	# app.run(ssl_context=('cert.pem', 'key.pem'))
