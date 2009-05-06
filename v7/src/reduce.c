/*  Implementation of aggregate functions.
    $Id$
    This file is part of Vlerq, see src/vlerq.h for full copyright notice. */

static vqType AggregateMax (vqType type, vqSlot column, vqSlot *item) {
    int r, rows;
    vqSlot temp;
    
    rows = columnp->count;
    if (rows == 0)
        return VQ_nil;
    
    *item = GetColItem(0, column, type);
    for (r = 1; r < rows; ++r) {
        temp = GetColItem(r, column, type);
        if (ItemsCompare (type, temp, *item, 0) > 0)
            *item = temp;
    }
    
    return type;
}

vqType MaxCmd_VN (vqSlot args[]) {
    vqSlot column = args[0].v->col[args[1].i];
    return AggregateMax(ViewColType(args[0].v, args[1].i), column, args);
}

static vqType AggregateMin (vqType type, vqSlot column, vqSlot *item) {
    int r, rows;
    vqSlot temp;
    
    rows = columnp->count;
    if (rows == 0)
        return VQ_nil;
    
    *item = GetColItem(0, column, type);
    for (r = 1; r < rows; ++r) {
        temp = GetColItem(r, column, type);
        if (ItemsCompare (type, temp, *item, 0) < 0)
            *item = temp;
    }
    
    return type;
}

vqType MinCmd_VN (vqSlot args[]) {
    vqSlot column = args[0].v->col[args[1].i];
    return AggregateMin(ViewColType(args[0].v, args[1].i), column, args);
}

static vqType AggregateSum (vqType type, vqSlot column, vqSlot *item) {
    int r, rows = columnp->count;
    
    switch (type) {
        case VQ_int:
            item->l = 0; 
            for (r = 0; r < rows; ++r)
                item->l += GetColItem(r, column, VQ_int).i;
            return VQ_long;
        case VQ_long:
            item->l = 0;
            for (r = 0; r < rows; ++r)
                item->l += GetColItem(r, column, VQ_long).l;
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

vqType SumCmd_VN (vqSlot args[]) {
    vqSlot column = args[0].v->col[args[1].i];
    return AggregateSum(ViewColType(args[0].v, args[1].i), column, &args[0]);
}