precision highp float;
out vec4 frag_color;

uniform float window_height;
uniform float offset_x;
uniform float offset_y;
uniform float scale;
uniform vec3 rgb_offsets;

const int max_iterations = 256;

bool in_main_cardioid(float real, float imaginary) {
    /* Tests whether the given value is in the main cardioid.
    */
    float imag2 = imaginary * imaginary;
    float u  = 1 + real * (8 * real*real + (16*imag2-3));

    float c0 = real * u + imag2 * (8*imag2-3);
    float c1 = (real+1) * (real+1) + imag2;

    return c0 < 3.0f / 32 || c1 < 1.0f / 16;
}

int divergence_test()
{
    float cr = gl_FragCoord.x / scale + offset_x;
    float ci = (window_height - gl_FragCoord.y) / scale + offset_y;

    if (in_main_cardioid(cr, ci)) 
    {
        return max_iterations;
    }

    float zr = 0.0;
    float zi = 0.0;

    int iterations = 0;

    while (zi*zi + zr*zr < 4 && iterations < max_iterations)
    {
        // z^2 = (a + bi)(a + bi) = a^2 - b^2 + 2abi
        float new_r = (zr*zr - zi*zi) + cr;
        zi = 2 * zr * zi + ci;
        zr = new_r;

        iterations++;
    }
    return iterations;
}

void main() 
{
    int iterations = divergence_test();

    vec3 colour = vec3(0.0, 0.0, 0.0);
    if (iterations != max_iterations) 
    {
        colour = vec3(
            0.5 * sin(0.1 * iterations + rgb_offsets.x) + 0.5,
            0.5 * sin(0.1 * iterations + rgb_offsets.y) + 0.5,
            0.5 * sin(0.1 * iterations + rgb_offsets.z) + 0.5);
    }

    frag_color = vec4(colour, 1.0);
}
