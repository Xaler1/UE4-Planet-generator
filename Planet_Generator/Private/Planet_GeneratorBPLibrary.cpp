// Copyright Epic Games, Inc. All Rights Reserved.

#include "Planet_GeneratorBPLibrary.h"
#include "Planet_Generator.h"
#include "math.h"
#include "stdlib.h"
#include "Kismet/KismetMathLibrary.h"
#include "time.h"

float coefs[11];

UPlanet_GeneratorBPLibrary::UPlanet_GeneratorBPLibrary(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	srand(time(NULL));
	for (int i = 0; i < 11; i++) {
		coefs[i] = rand() * 1000;
	}
}

FVector RThetaPhi_to_XYZ(float R, float Theta, float Phi);
FVector TranslateToSphere(FVector original, float radius);
FVector ApplyLandscape(FVector original, float radius, float wavelength, float amplitude, float offset, float Mountains, float Oceans);
float Landscape(float X);
float Mountains(float X);
float Sea(float X);
float Smooth(float X, float point, float rate);
float ThreeD_Perlin_Noise(float X, float Y, float Z, float wavelength, float amplitude);
float Interpolate(float a, float b, float x);
float Fade(float t);
float dotGradient(int X, float x, int Y = 0, float y = 0, int Z = 0, float z = 0, int W = 0, float w = 0);
FVector4 Random(int X, int Y, int Z, int W);

void UPlanet_GeneratorBPLibrary::GeneratePlanet(const float radius, const int divisions, const float wavelength, const float amplitude, const float offset, const float oceans, const float mountains, TArray<FVector>& Verticies, TArray<int32>& Triangles, TArray<FVector>& Normals) {
	int side_divisions = floor(divisions / 2);
	float step_size = sqrt((radius*radius) / 3) * 2 / (side_divisions - 1);
	FVector corners[6];
	FVector first_add_vectors[6];
	FVector second_add_vectors[6];
	corners[0] = FVector(sqrt((radius*radius) / 3), -sqrt((radius*radius) / 3), sqrt((radius*radius) / 3));
	corners[1] = FVector(sqrt((radius*radius) / 3), sqrt((radius*radius) / 3), sqrt((radius*radius) / 3));
	corners[2] = FVector(-sqrt((radius*radius) / 3), sqrt((radius*radius) / 3), sqrt((radius*radius) / 3));
	corners[3] = FVector(-sqrt((radius*radius) / 3), -sqrt((radius*radius) / 3), sqrt((radius*radius) / 3));
	corners[4] = FVector(sqrt((radius*radius) / 3), sqrt((radius*radius) / 3), sqrt((radius*radius) / 3));
	corners[5] = FVector(sqrt((radius*radius) / 3), -sqrt((radius*radius) / 3), -sqrt((radius*radius) / 3));
	first_add_vectors[0] = FVector(0.0f, 1.0f, 0.0f);
	second_add_vectors[0] = FVector(0.0f, 0.0f, -1.0f);
	first_add_vectors[1] = FVector(-1.0f, 0.0f, 0.0f);
	second_add_vectors[1] = FVector(0.0f, 0.0f, -1.0f);
	first_add_vectors[2] = FVector(0.0f, -1.0f, 0.0f);
	second_add_vectors[2] = FVector(0.0f, 0.0f, -1.0f);
	first_add_vectors[3] = FVector(1.0f, 0.0f, 0.0f);
	second_add_vectors[3] = FVector(0.0f, 0.0f, -1.0f);
	first_add_vectors[4] = FVector(0.0f, -1.0f, 0.0f);
	second_add_vectors[4] = FVector(-1.0f, 0.0f, 0.0f);
	first_add_vectors[5] = FVector(0.0f, 1.0f, 0.0f);
	second_add_vectors[5] = FVector(-1.0f, 0.0f, 0.0f);
	for (int i = 0; i < 6; i++) {
		for (int x = 0; x < side_divisions; x++) {
			for (int y = 0; y < side_divisions; y++) {
				Verticies.Add(ApplyLandscape(TranslateToSphere(corners[i] + first_add_vectors[i] * y * step_size + second_add_vectors[i] * x * step_size, radius), radius, wavelength, amplitude, offset, mountains, oceans));
				if (x < side_divisions - 1 && y < side_divisions - 1) {
					Triangles.Add(i * side_divisions * side_divisions + x * side_divisions + y);
					Triangles.Add(i * side_divisions * side_divisions + (x + 1) * side_divisions + y + 1);
					Triangles.Add(i * side_divisions * side_divisions + (x + 1) * side_divisions + y);
					Triangles.Add(i * side_divisions * side_divisions + x * side_divisions + y);
					Triangles.Add(i * side_divisions * side_divisions + x * side_divisions + y + 1);
					Triangles.Add(i * side_divisions * side_divisions + (x + 1) * side_divisions + y + 1);
				}
				if (x > 0 && y > 0) {
					FVector v1 = Verticies[i * side_divisions * side_divisions + (x - 1) * side_divisions + y - 1] - Verticies[i * side_divisions * side_divisions + x * side_divisions + y];
					FVector v2 = Verticies[i * side_divisions * side_divisions + (x - 1) * side_divisions + y] - Verticies[i * side_divisions * side_divisions + x * side_divisions + y];
					FVector normal = FVector(v1.Y * v2.Z - v1.Z * v2.Y, v1.Z * v2.X - v1.X * v2.Z, v1.X * v2.Y - v1.Y * v2.X);
					normal = Verticies[i * side_divisions * side_divisions + x * side_divisions + y].Size() > (Verticies[i * side_divisions * side_divisions + x * side_divisions + y] + normal).Size() ? normal : -normal;
					Normals.Add(normal);
				}
			}
		}
	}
}

FVector TranslateToSphere(FVector original, float radius) {
	return original / ((original.Size()) / radius);
}

FVector ApplyLandscape(FVector original, float radius, float wavelength, float amplitude, float offset, float mountains, float oceans) {
	float noise_large = ThreeD_Perlin_Noise(original.X + offset + radius, original.Y + offset + radius, original.Z + offset + radius, wavelength, amplitude);
	float noise_small = ThreeD_Perlin_Noise(original.X + offset + radius, original.Y + offset + radius, original.Z + offset + radius, wavelength / 2.0, amplitude / 3.0);
	float noise = powf(UKismetMathLibrary::FClamp(noise_large, 0.0, 1.0), mountains) - powf(abs(UKismetMathLibrary::FClamp(noise_large, -1.0, 0.0)), oceans);
	return original * (1 + (Landscape(noise) + noise_small)/(radius/10));
}

float Landscape(float X) {
	float h1 = Smooth(X, 0, 0.1);
	float h2 = Smooth(X, -0.4, 0.2);
	return -0.8 * (1 - h2) + Sea(X) * h2 * (1 - h1) + Mountains(X) * h1;
}

float Mountains(float X) {
	return exp(3.2 * X - 5) + X / 2;
}

float Sea(float X) {
	return pow((X + 1), 2) - 1;
}

float Smooth(float X, float point, float rate) {
	return 0.5 + 0.5 * tanh((X - point) / rate);
}

float ThreeD_Perlin_Noise(float x, float y, float z, float scale = 1, float amplitude = 1) {
	scale = scale <= 0 ? 1 : scale;
	x /= scale;
	y /= scale;
	z /= scale;
	int xL = floor(x);
	int xU = xL + 1;
	int yL = floor(y);
	int yU = yL + 1;
	int zL = floor(z);
	int zU = zL + 1;

	float dx = Fade(x - xL);
	float dy = Fade(y - yL);
	float dz = Fade(z - zL);

	return Interpolate(Interpolate(Interpolate(dotGradient(xL, x, yL, y, zL, z), dotGradient(xU, x, yL, y, zL, z), dx),
		Interpolate(dotGradient(xL, x, yU, y, zL, z), dotGradient(xU, x, yU, y, zL, z), dx), dy),
		Interpolate(Interpolate(dotGradient(xL, x, yL, y, zU, z), dotGradient(xU, x, yL, y, zU, z), dx),
			Interpolate(dotGradient(xL, x, yU, y, zU, z), dotGradient(xU, x, yU, y, zU, z), dx), dy), dz) * amplitude;
}

float Interpolate(float a, float b, float x) {
	float t = x * PI;
	float f = (1 - cos(t)) * 0.5;
	return a * (1 - f) + b * f;
}

FVector4 Random(int X, int Y, int Z, int W) {
	float seed = coefs[0] * sin(coefs[1] * X + coefs[2] * Y + coefs[3] * Z + coefs[4] * W + coefs[5]) * cos(coefs[6] * X + coefs[7] * Y + coefs[8] * Z + coefs[9] * W + coefs[10]);
	srand(seed);
	float x = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 2.0)) - 1.0;
	float y = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 2.0)) - 1.0;
	float z = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 2.0)) - 1.0;
	float w = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 2.0)) - 1.0;
	FVector4 random_vector = FVector4(x, y, z, w);
	return random_vector / random_vector.Size();
}

float Fade(float t) {
	return t * t * t * (t * (t * 6 - 15) + 10);
}

float dotGradient(int X, float x, int Y, float y, int Z, float z, int W, float w) {
	FVector4 random_vector = Random(X, Y, Z, W);
	float dx = x - X;
	float dy = y - Y;
	float dz = z - Z;
	float dw = w - W;
	return dx * random_vector.X + dy * random_vector.Y + dz * random_vector.Z + dw * random_vector.W;
}