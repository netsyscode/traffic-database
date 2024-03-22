from flask import Flask, request, send_file
from socketConnect import *
from pcapReader import *

app = Flask(__name__)
app.config['CSRF_ENABLED'] = False
s = socket_connect(SERVER_HOST,SERVER_PORT)
packets = []
meta_list = []
findPacket = False

@app.route('/search',methods=['POST'])
def search():
    global findPacket
    global packets
    global meta_list
    if request.method == 'POST':
        key = request.form['searchKey']
        
        send_pkt(s,key)
        
        if key == "q":
            return {'message': 'Querier quit.'}, 200
        
        response = recv_pkt(s)

        if response == "query fail.":
            findPacket = False
            return {'message': response, "data": []}, 200

        findPacket = True
        packets = get_packets()
        meta_list = get_meta_list(packets)

        return {'message': response, "data": meta_list}, 200
    else:
        return {'error': 'Only POST requests are allowed'}, 405
    
@app.route('/packet', methods=['POST'])
def get_packet():
    global findPacket
    global packets
    global meta_list
    if request.method == 'POST':
        if not findPacket:
            return {'message': 'no packet.', "meta": {}, "data":""}, 200
        num = int(request.form['packetNumber']) - 1
        if num > len(packets):
            return {'message': 'too big number.', "meta":{}, "data":""}, 200
        _, packet, _ = packets[num]
        meta = get_packet_meta(packet)
        data = ':'.join('{:02x}'.format(byte) for byte in packet)
        return {'message': 'get packet.', "meta":meta, "data":data}, 200
    else:
        return {'error': 'Only POST requests are allowed'}, 405
    
@app.route('/download', methods=['GET'])
def download():
    global findPacket
    global packets
    global meta_list
    file_path = "../../data/output/res.pcap"
    if request.method == 'GET':
        if not findPacket:
            return {'message': 'no packet.'}, 200
        write_pcap(file_path,packets)
        return send_file(file_path, as_attachment=True)
    else:
        return {'error': 'Only GET requests are allowed'}, 405

if __name__ == '__main__':
    app.run(host="127.0.0.1",port=8080)