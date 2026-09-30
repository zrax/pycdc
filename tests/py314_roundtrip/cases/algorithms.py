"""Classic algorithm scenarios: recursive + in-place quicksort, primality,
sieve of Eratosthenes, and RC4 KSA/PRGA — dense while/for/if nesting with
in-place swaps and XOR accumulation.

Observation targets (Python 3.13/3.14 bytecode):
 - while with two exit conditions (partition scan loop)
 - swap via tuple pack/unpack inside loops
 - augmented-assignment XOR chains
 - list comprehension + recursion mix (functional quicksort)
 - modulo-index walking (% len) in RC4 PRGA
"""
def quicksort(seq):
    if len(seq) <= 1:
        return list(seq)
    pivot = seq[len(seq) // 2]
    left = [x for x in seq if x < pivot]
    mid = [x for x in seq if x == pivot]
    right = [x for x in seq if x > pivot]
    return quicksort(left) + mid + quicksort(right)


def quicksort_inplace(seq):
    def sort(lo, hi):
        if lo < hi:
            p = partition(lo, hi)
            sort(lo, p)
            sort(p + 1, hi)

    def partition(lo, hi):
        pivot = seq[(lo + hi) // 2]
        i = lo - 1
        j = hi + 1
        while True:
            i += 1
            while seq[i] < pivot:
                i += 1
            j -= 1
            while seq[j] > pivot:
                j -= 1
            if i < j:
                (seq[i], seq[j]) = (seq[j], seq[i])
            else:
                return j
    sort(0, len(seq) - 1)
    return seq


def is_prime(n):
    if n < 2:
        return False
    if n < 4:
        return True
    if n % 2 == 0:
        return False
    d = 3
    while d * d <= n:
        if n % d == 0:
            return False
        d += 2
    return True


def primes_up_to(limit):
    sieve = [True] * (limit + 1)
    sieve[0] = False
    sieve[1] = False
    for i in range(2, int(limit ** 0.5) + 1):
        if sieve[i]:
            for j in range(i * i, limit + 1, i):
                sieve[j] = False
    return [n for n in range(limit + 1) if sieve[n]]


def rc4(key, data):
    s = list(range(256))
    j = 0
    for i in range(256):
        j = (j + s[i] + key[i % len(key)]) % 256
        (s[i], s[j]) = (s[j], s[i])
    out = bytearray()
    i = 0
    j = 0
    for b in data:
        i = (i + 1) % 256
        j = (j + s[i]) % 256
        (s[i], s[j]) = (s[j], s[i])
        out.append(b ^ s[(s[i] + s[j]) % 256])
    return bytes(out)


def __test__():
    data = [9, 3, 7, 1, 8, 2, 5, 5, 0, 6, 4]
    return [
        repr(quicksort(data)),
        repr(quicksort_inplace(list(data))),
        repr([is_prime(n) for n in [0, 1, 2, 3, 4, 17, 25, 97, 100, 101]]),
        repr(primes_up_to(30)),
        rc4(b'SecretKey', b'attack at dawn').hex(),
        repr(rc4(b'key', rc4(b'key', b'roundtrip check 123'))),
    ]
