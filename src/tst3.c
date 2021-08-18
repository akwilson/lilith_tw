static lval do_add(lval x, lval y)
{
    if (x.type == LVAL_LONG && y.type == LVAL_LONG)
    {
        return lval_long(x.value.num_l + y.value.num_l);
    }

    if (x.type == LVAL_LONG)
    {
        return lval_double(x.value.num_l + y.value.num_d);
    }

    if (y.type == LVAL_LONG)
    {
        return lval_double(x.value.num_d + y.value.num_l);
    }

    return lval_double(x.value.num_d + y.value.num_d);
}

