foobar :: () {
    return 1;
}

bar :: () -> i32;

main :: () {
    return bar();
}

bar :: () -> i32 {
    return 3;
}
