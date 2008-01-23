/*  Implementation of aggregate functions.
    $Id$
    This file is part of Vlerq, see src/vlerq.h for full copyright notice. */

static vqType AggregateMax (vqType type, vqCell column, vqCell *item) {
    int r, rows;
    vqCell temp;
    
    rows = columnp->count;
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

vqType MaxCmd_VN (vqCell args[]) {
    vqCell column = vwCol(args[0].v, args[1].i);
    return AggregateMax(ViewColType(args[0].v, args[1].i), column, args);
}

static vqType AggregateMin (vqType type, vqCell column, vqCell *item) {
    int r, rows;
    vqCell temp;
    
    rows = columnp->count;
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

vqType MinCmd_VN (vqCell args[]) {
    vqCell column = vwCol(args[0].v, args[1].i);
    return AggregateMin(ViewColType(args[0].v, args[1].i), column, args);
}

static vqType AggregateSum (vqType type, vqCell column, vqCell *item) {
    int r, rows = columnp->count;
    
    switch (type) {
        
        case VQ_int:
            item->w = 0; 
            for (r = 0; r < rows; ++r)
                item->w += GetColItem(r, column, VQ_int).i;
            return VQ_long;
            
        case VQ_long:
            item->w = 0;
            for (r = 0; r < rows; ++r)
                item->w += GetColItem(r, column, VQ_long).l;
            return VQ_long;
            
        case VQ_float:
            item->d = 0; 
            for (r = 0; r < rows; ++r)
                item->d += GetColItem(r, column, VQ_float).f;
            return VQ_double;

        case VQ_double:     
            item->d = 0; 
            for (r = 0; r < rows; ++r)
                item->d += GetColItem(r, column, VQ_double).d;
            return VQ_double;
            
        default:
            return VQ_nil;
    }
}

vqType SumCmd_VN (vqCell args[]) {
    vqCell column = vwCol(args[0].v, args[1].i);
    return AggregateSum(ViewColType(args[0].v, args[1].i), column, &args[0]);
}
