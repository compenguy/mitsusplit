#!./flask/bin/python

# Based on tutorial here:
# https://blog.miguelgrinberg.com/post/designing-a-restful-api-with-python-and-flask
from flask import Flask, request, jsonify, send_from_directory

app = Flask(__name__)

networks = [
    {
        "ssid": "gemini",
        "rssi": 88
    }
]

def send_from_dist(file):
    app.logger.info('sending web-provision/dist/%s', file)
    return send_from_directory('../web-provision/dist', file)

##### REST API #####
@app.route('/api/v1/wifi/connect', methods=['POST'])
def post_wifi_connect(args):
    if not request.json or not 'ssid' in request.json or 'psk' not in request.json:
        abort(400)
    network = {
        'ssid': request.json['ssid'],
        'psk': request.json['psk'],
    }
    app.logger.info('wifi connect requested for %s', {"network": network})
    return jsonify({'network': network}), 201

@app.route('/api/v1/wifi/scan', methods=['GET'])
def get_wifi_scan():
    app.logger.info('wifi scan returning %s', {"networks": networks})
    return jsonify({"networks": networks}), 201

##### SITE CONTENT #####
@app.route('/')
def root():
    return send_from_dist('index.html')

@app.route('/index.html')
def index():
    return send_from_dist(request.path[1:]);

@app.route('/favicon.ico')
def favicon():
    return send_from_dist(request.path[1:]);

@app.route('/axios.min.js')
def axios_js():
    return send_from_dist(request.path[1:]);

@app.route('/petite-vue.min.js')
def petite_vue_js():
    return send_from_dist(request.path[1:]);

@app.route('/pure-min.css')
def pure_css():
    return send_from_dist(request.path[1:]);

if __name__ == '__main__':
    app.run(debug=True)
