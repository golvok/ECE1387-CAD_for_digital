min: net_w_1 + net_w_2 + net_w_3 + net_h_1 + net_h_2 + net_h_3;

chip_xl = 0;
chip_xr = 3;
chip_yb = 0;
chip_yt = 3;

// upper and lower bounds on variables
chip_xl <= block_xl_A;
chip_yb <= block_yb_A;
block_xr_A <= chip_xr;
block_yt_A <= chip_yt;
chip_xl <= block_xl_B;
chip_yb <= block_yb_B;
block_xr_B <= chip_xr;
block_yt_B <= chip_yt;
chip_xl <= block_xl_C;
chip_yb <= block_yb_C;
block_xr_C <= chip_xr;
block_yt_C <= chip_yt;

0 <= net_w_1;
0 <= net_w_2;
0 <= net_w_3;

0 <= net_h_1;
0 <= net_h_2;
0 <= net_h_3;

// define the net widths & heights
net_w_1 = net_xr_1 - net_xl_1;
net_w_2 = net_xr_2 - net_xl_2;
net_w_3 = net_xr_3 - net_xl_3;

net_h_1 = net_yt_1 - net_yb_1;
net_h_2 = net_yt_2 - net_yb_2;
net_h_3 = net_yt_3 - net_yb_3;

// set block width and heights
1 = block_xr_A - block_xl_A;
1 = block_xr_B - block_xl_B;
2 = block_xr_C - block_xl_C;

1 = block_yt_A - block_yb_A;
2 = block_yt_B - block_yb_B;
1 = block_yt_C - block_yb_C;

// make some variables for the port locations
block_top_port_x_A = 0.5 block_xr_A + 0.5 block_xl_A;
block_top_port_y_A = block_yt_A;
block_right_port_x_A = block_xr_A;
block_right_port_y_A = 0.5 block_yt_A + 0.5 block_yb_A;

block_left_port_x_B = block_xl_B;
block_left_port_y_B = 0.5 block_yt_B + 0.5 block_yb_B;
block_right_port_x_B = block_xr_B;
block_right_port_y_B = 0.5 block_yt_B + 0.5 block_yb_B;

block_left_port_x_C = block_xl_C;
block_left_port_y_C = 0.5 block_yt_C + 0.5 block_yb_C;
block_right_port_x_C = block_xr_C;
block_right_port_y_C = 0.5 block_yt_C + 0.5 block_yb_C;
block_bottom_port_x_C = 0.5 block_xr_C + 0.5 block_xl_C;
block_bottom_port_y_C = block_yb_C;

// constrain net bounts based on port locations
// net 1 is the A-C one
// net 2 is the A-B-C one
// net 3 is the B-C one
block_top_port_x_A <= net_xr_1;
block_left_port_x_C <= net_xr_1;
block_top_port_y_A <= net_yt_1;
block_left_port_y_C <= net_yt_1;
net_xl_1 <= block_top_port_x_A;
net_xl_1 <= block_left_port_x_C;
net_yb_1 <= block_top_port_y_A;
net_yb_1 <= block_left_port_y_C;

block_right_port_x_A <= net_xr_2;
block_left_port_x_B <= net_xr_2;
block_bottom_port_x_C <= net_xr_2;
block_right_port_y_A <= net_yt_2;
block_left_port_y_B <= net_yt_2;
block_bottom_port_y_C <= net_yt_2;
net_xl_2 <= block_left_port_x_B;
net_xl_2 <= block_right_port_x_A;
net_xl_2 <= block_bottom_port_x_C;
net_yb_2 <= block_right_port_y_A;
net_yb_2 <= block_left_port_y_B;
net_yb_2 <= block_bottom_port_y_C;

block_right_port_x_B <= net_xr_3;
block_right_port_x_C <= net_xr_3;
block_right_port_y_B <= net_yt_3;
block_right_port_y_C <= net_yt_3;
net_xl_3 <= block_right_port_x_B;
net_xl_3 <= block_right_port_x_C;
net_yb_3 <= block_right_port_y_B;
net_yb_3 <= block_right_port_y_C;

// restrict overlaps
rileft_1_2 + rileft_2_1 + bodtop_1_2 + bodtop_2_1 <= 3;
rileft_1_3 + rileft_3_1 + bodtop_1_3 + bodtop_3_1 <= 3;
rileft_2_3 + rileft_3_2 + bodtop_2_3 + bodtop_3_2 <= 3;

block_xr_A <= block_xl_B + 40 rileft_1_2;
block_xr_A <= block_xl_C + 40 rileft_1_3;
block_xr_B <= block_xl_A + 40 rileft_2_1;
block_xr_B <= block_xl_C + 40 rileft_2_3;
block_xr_C <= block_xl_A + 40 rileft_3_1;
block_xr_C <= block_xl_B + 40 rileft_3_2;

block_yt_A <= block_yb_B + 40 bodtop_1_2;
block_yt_A <= block_yb_C + 40 bodtop_1_3;
block_yt_B <= block_yb_A + 40 bodtop_2_1;
block_yt_B <= block_yb_C + 40 bodtop_2_3;
block_yt_C <= block_yb_A + 40 bodtop_3_1;
block_yt_C <= block_yb_B + 40 bodtop_3_2;

/****
 * Begin type declarations
 ****/
// int block_xl_A, block_xl_B, block_xl_C;
// int block_xr_A, block_xr_B, block_xr_C;
// int block_yb_A, block_yb_B, block_yb_C;
// int block_yt_A, block_yt_B, block_yt_C;

bin rileft_1_2, rileft_1_3;
bin rileft_2_1, rileft_2_3;
bin rileft_3_1, rileft_3_2;
bin bodtop_1_2, bodtop_1_3;
bin bodtop_2_1, bodtop_2_3;
bin bodtop_3_1, bodtop_3_2;
