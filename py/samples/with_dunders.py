
class poo:
    def __init__(self):
        print('constructing poo')
    def __del__(self):
        print('destructing poo')
    def __enter__(self):
        print('entering poo')
        return 'this is a string'
    def __exit__(self, exc_type, exc_val, exc_tb):
        print(f'exiting poo {exc_type, exc_val, exc_tb}')

p = poo()
with p as o:
    print(f'inside with statement, o is: "{o}"')
