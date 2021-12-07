/* posconverter
 * A positional notation converter
 */

/* Returns a function which decodes a positional notation string into decimal.
   Takes the positional notation's base and a table of characters to values
   to represent digits above 9.
*/
function decode_pos_notation(base, symbol_table = NaN) {
    return (string) => {
        // Reverse the string to access the least significant digits first,
        // and convert to upper case
        string = string.split("")
            .reverse()
            .join("")
            .toUpperCase();
       
        let result = 0;

        for (let i = 0; i < string.length; i++) {
            let character = string[i];

            // If the char isn't a digit, search the table for its value
            let as_num = parseFloat(character);
            let digit_value = isNaN(as_num)
                ? symbol_table[character]
                : as_num;
            
            // place values are the digit value multiplied with the base raised 
            // to the power of the place.
            // For example the 2 in 200 in decimal has a value of 2 * 10 ^ 3
            let place_value = digit_value * Math.pow(base, i);
            result += place_value;
        }

        return result;
    };
}

function encode_pos_notation(base, symbol_table = NaN) {
    return (decimal) => {
        // Number of bits needed to represent the given decimal, and bits
        // needed to represent one digit of the resulting pos notation
        let length = Math.floor(Math.log2(decimal) + 1);
        let digit_bits = Math.round(Math.log2(base)); 

        // Bitmask to extract values. Gives a binary number
        // consisting of all 1's with a length of digit_bits
        let mask = Math.pow(2, digit_bits) - 1;

        let result = "";

        for (let i = 0; i < length; i += digit_bits) {
            let value = (decimal >> i) & mask; // Value at place i

            // If the value can not be represented with a digit, search the table
            let char = value > 9 ? symbol_table[value] : value;
            result = char + result;
        }

        return result;
    };
}

function encode_decode(in_base, out_base, in_table = NaN, out_table = NaN) {
    return (string) => {
        let dec = decode_pos_notation(in_base, in_table)(string);
        return encode_pos_notation(out_base, out_table)(dec);
    };
}

function convert_input() {
    const hex_to_dec_table = {
        "A": 10, "B": 11, "C": 12,
        "D": 13, "E": 14, "F": 15
    };
    const dec_to_hex_table = {
        10: "A", 11: "B", 12: "C",
        13: "D", 14: "E", 15: "F"
    };
    const options = {
        "dec-hex": encode_pos_notation(16, dec_to_hex_table),
        "hex-dec": decode_pos_notation(16, hex_to_dec_table),
        "dec-bin": encode_pos_notation(2),
        "bin-dec": decode_pos_notation(2),
        "hex-bin": encode_decode(16, 2, hex_to_dec_table, NaN),
        "bin-hex": encode_decode(2, 16, NaN, dec_to_hex_table)
    };
    let option = document.getElementById("options").value;
    let input = document.getElementById("input").value;
    let output = options[option](input);

    document.getElementById("output").setAttribute("value", output);
}

window.onload = () => {
   let convert = document.getElementById("convert");
   convert.addEventListener("click", convert_input);
};
