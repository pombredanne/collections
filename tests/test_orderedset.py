from bcse.collections import orderedset

if __name__ == '__main__':
    a = orderedset('abracadabra')
    b = orderedset('ommanipadmehum')
    print(a)
    print(b)
    print(a < b)
    print(a <= b)
    print(a == b)
    print(a >= b)
    print(a > b)
    print(a | b)
    print(b | a)
    print(a & b)
    print(b & a)
    print(a - b)
    print(b - a)
    print(a ^ b)
    print(b ^ a)

    print('Benchmark...')

    from random import randint
    from time import time

    data1 = [randint(1, 100000) for _ in range(100000)]
    data2 = [randint(1, 100000) for _ in range(100000)]

    t0 = time()
    orderedset(data1)
    t = time() - t0
    print('init with 100k random integers: %fs' % t)

    a = orderedset(data1)
    b = orderedset(data2)
    t0 = time()
    c = a | b
    t = time() - t0
    print('a | b: %fs' % t)

    a = orderedset(data1)
    b = orderedset(data2)
    t0 = time()
    c = a & b
    t = time() - t0
    print('a & b: %fs' % t)

    a = orderedset(data1)
    b = orderedset(data2)
    t0 = time()
    c = a - b
    t = time() - t0
    print('a - b: %fs' % t)

    a = orderedset(data1)
    b = orderedset(data2)
    t0 = time()
    c = a ^ b
    t = time() - t0
    print('a ^ b: %fs' % t)

    a = orderedset(data1)
    b = orderedset(data2)
    t0 = time()
    a |= b
    t = time() - t0
    print('a |= b: %fs' % t)

    a = orderedset(data1)
    b = orderedset(data2)
    t0 = time()
    a &= b
    t = time() - t0
    print('a &= b: %fs' % t)

    a = orderedset(data1)
    b = orderedset(data2)
    t0 = time()
    a -= b
    t = time() - t0
    print('a -= b: %fs' % t)

    a = orderedset(data1)
    b = orderedset(data2)
    t0 = time()
    a ^= b
    t = time() - t0
    print('a ^= b: %fs' % t)

    a = orderedset()
    t0 = time()
    [a.add(i) for i in data1]
    t = time() - t0
    print('add 100k times: %fs' % t)

    a = orderedset(data1)
    len = len(a)
    b = range(len-1, -1, -1)
    t0 = time()
    [a.__delitem__(i) for i in b]
    t = time() - t0
    print('reversed sequential delete %d items: %fs' % (len, t))

    a = orderedset(data1)
    t0 = time()
    [a.discard(i) for i in data2]
    t = time() - t0
    print('discard 100k times: %fs' % t)

    a = orderedset(data1)
    t0 = time()
    while 1:
        try:
            a.pop()
        except IndexError:
            break
    t = time() - t0
    print('pop all items: %fs' % t)

    a = orderedset(data1)
    t0 = time()
    c = list(a)
    t = time() - t0
    print('list(a): %fs' % t)

    a = orderedset(data1)
    t0 = time()
    c = reversed(a)
    t = time() - t0
    print('reversed(a): %fs' % t)

    a = orderedset(data1)
    b = orderedset(data2)
    t0 = time()
    [(i in a) for i in b]
    t = time() - t0
    print('[(i in a) for i in b]: %fs' % t)
