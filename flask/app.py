from flask import Flask
from flask_restful import Resource, Api, reqparse
from waitress import serve
import sqlite3
import sys

connection = sqlite3.connect('nodedb.db')
with open('nodeschema.sql') as f:
    connection.executescript(f.read())
connection.commit()
connection.close()
app = Flask(__name__)
api = Api(app)

# this guide was used for guidance:
# https://towardsdatascience.com/the-right-way-to-build-an-api-with-python-cd08ab285f8f


def get_db_connection():
    conn = sqlite3.connect('nodedb.db')
    return conn


class Register(Resource):
    def get(self):
        conn = get_db_connection()
        cur = conn.cursor()
        cur.execute('INSERT INTO nodes (severity) VALUES (0);')
        node_id = cur.lastrowid
        conn.commit()
        cur.close()
        print('Registered node with id: ' + str(node_id), file=sys.stderr)
        return node_id

    def delete(self):
        parser = reqparse.RequestParser()
        parser.add_argument('id', required=True)
        args = parser.parse_args()

        conn = get_db_connection()
        cur = conn.cursor()
        cur.execute('DELETE FROM nodes WHERE id = ?', (args['id']))
        conn.commit()
        cur.close()
        print('Deleted node with id: ' + args['id'], file=sys.stderr)
        return 200


class Nodes(Resource):
    def get(self):
        conn = get_db_connection()
        cur = conn.cursor()
        rows = cur.execute('SELECT severity FROM nodes').fetchall()
        conn.commit()
        cur.close()
        severities = ""
        for row in rows:
            severities += str(row[0])
            severities += ','
        print('Received request for node severities', file=sys.stderr)
        return severities

    def put(self):
        parser = reqparse.RequestParser()
        parser.add_argument('id', required=True)
        parser.add_argument('severity', required=True)
        args = parser.parse_args()

        conn = get_db_connection()
        cur = conn.cursor()
        cur.execute("UPDATE nodes SET severity = ?, lastUpdate = datetime('now') WHERE id = ?;", (args['severity'], args['id']))
        conn.commit()
        cur.close()
        print('node with id ' + args['id'] + ' reported severity of ' + args['severity'], file=sys.stderr)
        return 200


class FaultCheck(Resource):
    def put(self):
        conn = get_db_connection()
        cur = conn.cursor()
        cur.execute("DELETE FROM nodes WHERE lastUpdate < datetime('now', '-10 second');")
        conn.commit()
        cur.close()
        return 200


api.add_resource(Register, '/register')
api.add_resource(Nodes, '/nodes')
api.add_resource(FaultCheck, '/faultcheck')

if __name__ == '__main__':
    serve(app, host='0.0.0.0', port=8080)
