int a, b, c, i;
a = 5;
b = 10;
c = a + b * 2;

if (c > 15) {
    c = c - 1;
} else {
    c = c + 1;
}

i = 0;
while (i < 3) {
    c = c + i;
    i = i + 1;
}