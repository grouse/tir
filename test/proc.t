foobar :: () {
    return 1;
}

bar :: () -> i32;

#foreign hello_world :: () -> i32;

main :: () {
    hello_world();
    return 2;
}

bar :: () -> i32 {
    return 3;
}
