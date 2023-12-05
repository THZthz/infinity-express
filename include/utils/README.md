[trivially copyable](#trivially-copyable)


## Trivially copyable

```c++
// empty classes are trivial
struct Trivial1 {};

// all special members are implicit
struct Trivial2 {
    int x;
};

struct Trivial3 : Trivial2 { // base class is trivial
    Trivial3() = default; // not a user-provided ctor
    int y;
};

struct Trivial4 {
public:
    int a;
private: // no restrictions on access modifiers
    int b;
};

struct Trivial5 {
    Trivial1 a;
    Trivial2 b;
    Trivial3 c;
    Trivial4 d;
};

struct Trivial6 {
    Trivial2 a[23];
};

struct Trivial7 {
    Trivial6 c;
    void f(); // it's okay to have non-virtual functions
};

struct Trivial8 {
    int x;
    static NonTrivial1 y; // no restrictions on static members
}

struct Trivial9 {
    Trivial9() = default; // not user-provided
    // a regular constructor is okay because we still have default ctor
    Trivial9(int x) : x(x) {};
    int x;
}

struct NonTrivial1 : Trivial3 {
    virtual f(); // virtual members make non-trivial ctors
}

struct NonTrivial2 {
    NonTrivial2() : z(42) {} // user-provided ctor
    int z;
}

struct NonTrivial3 {
    NonTrivial3(); // user-provided ctor
    int w;
}
NonTrivial3::NonTrivial3() = default; // defaulted but not on first declaration still counts as user-provided

struct NonTrivial5 {
    virtual ~NonTrivial5(); // virtual destructors are not trivial
};
```
