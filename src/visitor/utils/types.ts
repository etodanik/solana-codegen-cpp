enum NumberType {
    i8 = "i8",
    u8 = "u8",
    i16 = "i16",
    u16 = "u16",
    i32 = "i32",
    u32 = "u32",
    i64 = "i64",
    u64 = "u64",
    i128 = "i128",
    u128 = "u128",
    isize = "isize",
    usize = "usize",
}

export function numberFormatToCppType(numberType: NumberType) {
    switch (numberType) {
        case NumberType.i8:
            return "int8";
        case NumberType.u8:
            return "uint8";
        case NumberType.i16:
            return "int16";
        case NumberType.u16:
            return "uint16";
        case NumberType.i32:
            return "int32";
        case NumberType.u32:
            return "uint32";
        case NumberType.i64:
            return "int64";
        case NumberType.u64:
            return "uint64";
        case NumberType.i128:
            return "int128";
        case NumberType.u128:
            return "uint128";
        case NumberType.isize:
            return "size_t";
        case NumberType.usize:
            return "size_t";
        default:
            throw new Error(`Number type not supported: ${numberType}`);
    }
}
