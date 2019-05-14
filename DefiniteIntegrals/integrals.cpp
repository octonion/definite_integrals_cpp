#include <emmintrin.h>
#include <iostream>

#include "integrals.h"

real_t definite_integral_rectangles(function_t function, real_t a, real_t b, precision_t nRects)
{
	real_t step = (b - a) / (real_t)nRects;
	real_t half_step = step * 0.5f;

	real_t i = 0.0f;
	int nIt = 0;
	for (real_t s = a + half_step; s < b; s += step)
		i += function(s) * step;
	return i;
}

real_t definite_integral_rectangles_sse(function_t function, real_t a, real_t b, precision_t nRects)
{
#ifndef NDEBUG
	// Rounds to the closest multiple of 4.
	int rem = nRects % 4;
	if (rem > 0)
		nRects += rem;
#endif
	real_t step = (b - a) / (real_t)nRects;
	real_t half_step = step * 0.5f;

	__m128 mm_int = { 0.0f, 0.0f, 0.0f, 0.0f };
	__m128 mm_step = { step, step, step, step };
	__m128 heights;

	int nIts = 0;
	for (real_t s = a + half_step; s < b; s = step * 4 + s)
	{
		heights =
		{
			function(s),
			function(step + s),
			function(step * 2 + s),
			function(step * 3 + s)
		};
		
		mm_int = _mm_add_ps(mm_int, _mm_mul_ps(heights, mm_step));
	}

	float __declspec(align(16)) int_array[4];
	_mm_store_ps(int_array, mm_int);
	return int_array[0] + int_array[1] + int_array[2] + int_array[3];
}


real_t definite_integral_cs(function_t function, real_t a, real_t b, precision_t nIntervals)
{
#ifndef NDEBUG
	if (nIntervals % 2 != 0)
		++nIntervals;
#endif

	real_t h = (b - a) / (real_t)nIntervals;
	real_t x0, y0, x1, y1, x2, y2;
	real_t i = 0.0f;
	real_t h_3 = h / 3.f;
	y0 = function(a);
	for (real_t s = a; s < b; s += h * 2)
	{
		x1 = s + h;
		y1 = function(x1);
		x2 = h * 2 + s;
		y2 = function(x2);

		i += h_3 * (y0 + (4 * y1) + y2);
		y0 = y2;
	}
	return i;
}

real_t definite_integral_cs_sse(function_t function, real_t a, real_t b, precision_t nIntervals)
{
#ifndef NDEBUG
	int rem = nIntervals % 8;
	if (rem > 0)
		nIntervals += rem;
#endif

	// Utility data (compute once to save CPU time)
	real_t h = (b - a) / (real_t)nIntervals;
	real_t h_3 = h / 3.f;

	// 128bit accumulator
	__m128 mm_int = { 0.0f, 0.0f, 0.0f, 0.0f };
	__m128 p;
	float __declspec(align(16)) mm_y[12];
	
	float y0, y1, y2, y3, y4, y5, y6, y7, y8;
	y0 = function(a);

	for (real_t s = a; s < b; s += h * 8)
	{
		// Computes function values.
		y1 = function(s + h);
		y2 = function(s + h * 2);
		y3 = function(s + h * 3);
		y4 = function(s + h * 4);
		y5 = function(s + h * 5);
		y6 = function(s + h * 6);
		y7 = function(s + h * 7);
		y8 = function(s + h * 8);

		// SIMD sum of partial results.
		p =
		{
			h_3 * (y0 + (4 * y1) + y2),
			h_3 * (y2 + (4 * y3) + y4),
			h_3 * (y4 + (4 * y5) + y6),
			h_3 * (y6 + (4 * y7) + y8)
		};

		mm_int = _mm_add_ps(mm_int, p);
		y0 = y8;
	}

	float __declspec(align(16)) int_array[4];
	_mm_store_ps(int_array, mm_int);
	return int_array[0] + int_array[1] + int_array[2] + int_array[3];
}


real_t gaussian_prob_sse(float mean, float stdev, real_t a, real_t b, precision_t nRects)
{
#ifndef NDEBUG
	int rem = nRects % 4;
	if (rem > 0)
		nRects += rem;
#endif
	real_t h = (b - a) / (real_t)nRects;
	real_t h_2 = h * 0.5f;
	__m128 mm_int = { 0.0f, 0.0f, 0.0f, 0.0f };
	__m128 mm;
	__m128 mm_h = { h, h, h, h };

	// Computes once the common coefficient of the Gauss function.
	real_t coeff = 1 / (stdev * sqrt(2 * 3.141f));
	real_t var = stdev * stdev;
	real_t errors[4];
	__m128 mm_coeff = { coeff, coeff, coeff, coeff };

	for (real_t s = a + h_2; s < b; s += h * 4)
	{
		// Computing errors
		errors[0] = s - mean;
		errors[1] = s + h - mean;
		errors[2] = s + h * 2 - mean;
		errors[3] = s + h * 3 - mean;
	
		// Computing exponentials
		mm =
		{
			exp(-0.5f * errors[0] * errors[0] / var),
			exp(-0.5f * errors[1] * errors[1] / var),
			exp(-0.5f * errors[2] * errors[2] / var),
			exp(-0.5f * errors[3] * errors[3] / var)
		};

		// Computing area of rects
		mm = _mm_mul_ps(mm, mm_coeff);
		mm_int = _mm_add_ps(mm_int, _mm_mul_ps(mm, mm_h));
	}

	// Retrieves result.
	float __declspec(align(16)) int_array[4];
	_mm_store_ps(int_array, mm_int);
	return int_array[0] + int_array[1] + int_array[2] + int_array[3];
}