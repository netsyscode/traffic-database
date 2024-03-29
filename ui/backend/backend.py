from flask import Flask, request, send_file
from socketConnect import *
from pcapReader import *
from flask_cors import CORS
from dataTrans import *
import argparse

app = Flask(__name__)
app.config['CSRF_ENABLED'] = False
CORS(app)
s = socket_connect(SERVER_HOST,SERVER_PORT)
packets = []
meta_list = []
findPacket = False
eth_lenth = 14

@app.route('/search',methods=['POST'])
def search():
    global findPacket
    global packets
    global meta_list
    if request.method == 'POST':
        key = request.form['searchKey']
        start_time = translate(request.form['startTime'])
        end_time = translate(request.form['endTime'])
        print(start_time+'\n'+end_time+'\n'+key)
        
        send_pkt(s,start_time+'\n'+end_time+'\n'+key)
        
        if key == "q":
            return {'message': 'Querier quit.'}, 200
        
        response = recv_pkt(s)
        #response = "query done."

        if response == "query fail.":
            findPacket = False
            return {'message': response, "data": []}, 200

        findPacket = True
        packets = get_packets()
        meta_list = get_meta_list(packets,eth_lenth)

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
        meta = get_packet_meta(packet,eth_lenth)
        # data = ' '.join('{:02x}'.format(byte) for byte in packet)

        line_break_interval = 16
        data = ''
        i = 0
        for byte in packet:
            if(i % line_break_interval == 0):
                data += '0x'
                data += '{:04x}'.format(i)
                data += ': '
            data += '{:02x}'.format(byte)
            if (i + 1) % line_break_interval == 0 and i != len(packet) - 1:
                data += '\n'
            else:
                data += ' '
            i+=1
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
        write_pcap(file_path,packets,eth_lenth)
        return send_file(file_path, as_attachment=True)
    else:
        return {'error': 'Only GET requests are allowed'}, 405

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-e', '--eth_header_len', type=int, help='Ethernet header length')
    # parser.add_argument('-f', '--filename', type=str, help='Input filename')

    args = parser.parse_args()
    if args.eth_header_len is not None:
        eth_lenth = args.eth_header_len
    app.run(host="127.0.0.1",port=8080)