"""
Contains various utility functions
"""

def argsplit(str, delim=' ', aquote='`'):
    " Splits a string similar to command line arguments "
    quote = False
    escaped = False
    ret = ['']
    for c in str:
        
        if c in '\\' and not escaped:
            escaped = True
            c = ''
        elif c in aquote and not escaped:
            quote = not quote
            c = ''
        elif escaped:
            escaped = False
            
        if c in delim and not quote:
            if ret[-1] != '' : ret.append('')
        else:
            ret[-1] += c
        
    return ret
        
    