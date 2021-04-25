#ifndef GDTERM_POSITION_HPP
#define GDTERM_POSITION_HPP

struct position {
    int x;
    int y;

    friend bool operator==(position left, position right)
    {
        return left.x == right.x && left.y == right.y;
    }

    friend bool operator!=(position left, position right)
    {
        return !(left == right);
    }
};

#endif // header guard
