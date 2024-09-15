module NCO #(
    parameter OUT_SIZE,
    parameter ACC_SIZE
)(
    input wire clock,
    input wire [ACC_SIZE-1 : 0] tuning_word,
    output reg [OUT_SIZE-1 : 0] signal
);

parameter OUT_MAX = integer'($pow(2, OUT_SIZE) / 2);
parameter ACC_MAX = integer'($pow(2, ACC_SIZE));

reg [OUT_SIZE-1 : 0] phase_to_amp [ACC_MAX-1 : 0];
reg [ACC_SIZE-1 : 0] phase_acc = 0;

initial begin
    integer i;
    for (i = 0; i < ACC_MAX; i=i+1) begin
        real scaled = $sin(real'(i) / ACC_MAX * 2.0 * 3.14) * OUT_MAX;
        phase_to_amp[i] = OUT_SIZE'(integer'(scaled));
    end
    signal = 0;
end

always @(posedge clock) begin
    phase_acc <= phase_acc + tuning_word;    
    signal <= phase_to_amp[phase_acc];
end

endmodule

