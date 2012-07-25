"""
DBProtocol - The SimpleDB protcol handler.
"""


import simpledb

class DBProtocol():
    
    def __init__(self, client):
        self.client = client

    def cmd_OHHI(self, args):
        try:
            self.client.sock.send('WELCOME\r\n')
            self.client.init = True
        except simpledb.error, detail:
            self.request_fail(str(detail))

    def cmd_INSERT(self, args):
        if not self.client.init:
            self.request_fail('Send OHHI first')
            return 
        try:
            simpledb.insert(args[0], args[1], long(args[2]))
            self.result_ok()
        except IndexError:
            self.request_fail("Not enough arguments (expected 3)")
        except simpledb.error, detail:
            self.result_fail(str(detail))

    def cmd_UPDATE(self, args):
        if not self.client.init:
            self.request_fail('Send OHHI first')
            return 
        try:
            simpledb.update(int(args[0]), args[1], args[2], long(args[3]))
            self.result_ok()
        except OverflowError:
            self.request_fail("Value too large: " + args[3])
        except IndexError:
            self.request_fail("Not enough arguments (expected 4)")
        except simpledb.error, detail:
            self.result_fail(str(detail))

    def cmd_FIND(self, args):
        if not self.client.init:
            self.request_fail('Send OHHI first')
            return
        try:
            record = simpledb.find(long(args[0]))
            record = [str(i) for i in record]
            record[0] = record[0].replace('`', '\\`')
            record[1] = record[1].replace('`', '\\`')
            record[0] = self.quote_string(record[0])
            record[1] = self.quote_string(record[1])
            record.insert(0, str(args[0]))
            record.insert(0, 'RESULT');
            response = ' '.join(record)
            response += '\r\n'

            self.client.sock.send(response)
        except IndexError:
            self.request_fail("Not enough arguments (expected 1)")
        except TypeError:
            self.request_fail("Argument must be an integer")
        except simpledb.error, detail:
            self.result_fail(str(detail))
        except:
            self.result_fail()

    def quote_string(self, string, quote='`'):
        return ''.join([quote, string, quote])

    def result_ok(self):
        self.client.sock.send('RESULT OK\r\n')

    def result_fail(self, reason=''):
        response = ' '.join(['RESULT FAIL', reason])
        print self.client, response
        response += '\r\n'
        self.client.sock.send(response)

    def request_fail(self, reason=''):
        response = ' '.join(['FAIL', reason])
        print self.client, response
        response += '\r\n'
        self.client.sock.send(response)

