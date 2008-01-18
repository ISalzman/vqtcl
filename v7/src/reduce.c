/*  Implementation of aggregate functions.
    $Id$
    This file is part of Vlerq, see src/vlerq.h for full copyright notice. */

static ItemTypes AggregateMax (ItemTypes type, Column column, Item_p item) {
    int r, rows;
    Item temp;
    
    rows = column.seq->count;
    if (rows == 0) {
        item->e = EC_nalor;
        return IT_error;
    }
    
    *item = GetColItem(0, column, type);
    for (r = 1; r < rows; ++r) {
        temp = GetColItem(r, column, type);
        if (ItemsCompare (type, temp, *item, 0) > 0)
            *item = temp;
    }
    
    return type;
}

ItemTypes MaxCmd_VN (Item args[]) {
    Column column = ViewCol(args[0].v, args[1].i);
    return AggregateMax(ViewColType(args[0].v, args[1].i), column, args);
}

static ItemTypes AggregateMin (ItemTypes type, Column column, Item_p item) {
    int r, rows;
    Item temp;
    
    rows = column.seq->count;
    if (rows == 0) {
        item->e = EC_nalor;
        return IT_error;
    }
    
    *item = GetColItem(0, column, type);
    for (r = 1; r < rows; ++r) {
        temp = GetColItem(r, column, type);
        if (ItemsCompare (type, temp, *item, 0) < 0)
            *item = temp;
    }
    
    return type;
}

ItemTypes MinCmd_VN (Item args[]) {
    Column column = ViewCol(args[0].v, args[1].i);
    return AggregateMin(ViewColType(args[0].v, args[1].i), column, args);
}

static ItemTypes AggregateSum (ItemTypes type, Column column, Item_p item) {
    int r, rows = column.seq->count;
    
    switch (type) {
        
        case IT_int:
            item->w = 0; 
            for (r = 0; r < rows; ++r)
                item->w += GetColItem(r, column, IT_int).i;
            return IT_wide;
            
        case IT_wide:
            item->w = 0;
            for (r = 0; r < rows; ++r)
                item->w += GetColItem(r, column, IT_wide).w;
            return IT_wide;
            
        case IT_float:
            item->d = 0; 
            for (r = 0; r < rows; ++r)
                item->d += GetColItem(r, column, IT_float).f;
            return IT_double;

        case IT_double:     
            item->d = 0; 
            for (r = 0; r < rows; ++r)
                item->d += GetColItem(r, column, IT_double).d;
            return IT_double;
            
        default:
            return IT_unknown;
    }
}

ItemTypes SumCmd_VN (Item args[]) {
    Column column = ViewCol(args[0].v, args[1].i);
    return AggregateSum(ViewColType(args[0].v, args[1].i), column, &args[0]);
}
