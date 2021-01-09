// Copyright Epic Games, Inc. All Rights Reserved.

#include "Planet_GeneratorBPLibrary.h"
#include "Planet_Generator.h"
#include "math.h"
#include "stdlib.h"
#include "Kismet/KismetMathLibrary.h"
#include "time.h"

int p[512];

UPlanet_GeneratorBPLibrary::UPlanet_GeneratorBPLibrary(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	srand(time(NULL));
	for (int i = 0; i < 512; i++) {
		p[i] = rand() % 256 + 1;
	}
}

FVector rThetaPhi_to_XYZ(float R, float Theta, float Phi);
FVector translateToSphere(FVector original, float radius);
FVector applyLandscape(FVector original, float radius, float wavelength, float amplitude, float offset, float Mountains, float Oceans);
float landscape(float X);
float mountains(float X);
float sea(float X);
float smooth(float X, float point, float rate);
float ThreeD_Perlin_Noise(float X, float Y, float Z, float wavelength, float amplitude);
float Interpolate(float a, float b, float x);
float Fade(float t);
float Gradient(int hash, float X, float Y, float Z);

void UPlanet_GeneratorBPLibrary::GeneratePlanet(const float radius, const int divisions, const float wavelength, const float amplitude, const float offset, const float Oceans, const float Mountains, TArray<FVector>& Verticies, TArray<int32>& Triangles, TArray<FVector>& Normals) {
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
				Verticies.Add(applyLandscape(translateToSphere(corners[i] + first_add_vectors[i] * y * step_size + second_add_vectors[i] * x * step_size, radius), radius, wavelength, amplitude, offset, Mountains, Oceans));
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

FVector rThetaPhi_to_XYZ(float R, float Theta, float Phi) {
	return FVector(R * sin(Theta) * cos(Phi), R * sin(Theta) * sin(Phi), R * cos(Theta));
}

FVector translateToSphere(FVector original, float radius) {
	return original / ((original.Size()) / radius);
}

FVector applyLandscape(FVector original, float radius, float wavelength, float amplitude, float offset, float Mountains, float Oceans) {
	float noise_large = ThreeD_Perlin_Noise(original.X + offset + radius, original.Y + offset + radius, original.Z + offset + radius, wavelength, amplitude);
	float noise_small = ThreeD_Perlin_Noise(original.X + offset + radius, original.Y + offset + radius, original.Z + offset + radius, wavelength / 2.0, amplitude / 3.0);
	float noise = powf(UKismetMathLibrary::FClamp(noise_large, 0.0, 1.0), Mountains) - powf(abs(UKismetMathLibrary::FClamp(noise_large, -1.0, 0.0)), Oceans);
	return original * (1 + (landscape(noise) + noise_small)/(radius/10));
}

float landscape(float X) {
	float h1 = smooth(X, 0, 0.1);
	float h2 = smooth(X, -0.4, 0.2);
	return -0.8 * (1 - h2) + sea(X) * h2 * (1 - h1) + mountains(X) * h1;
}

float mountains(float X) {
	return exp(3.2 * X - 5) + X / 2;
}

float sea(float X) {
	return pow((X + 1), 2) - 1;
}

float smooth(float X, float point, float rate) {
	return 0.5 + 0.5 * tanh((X - point) / rate);
}

float ThreeD_Perlin_Noise(float X, float Y, float Z, float wavelength = 1, float amplitude = 1) {
	if (wavelength == 0) {
		wavelength = 1;
	}
	X /= wavelength;
	Y /= wavelength;
	Z /= wavelength;
	int a = int(floor(X)) & 0xff;
	int b = int(floor(Y)) & 0xff;
	int c = int(floor(Z)) & 0xff;
	X -= floor(X);
	Y -= floor(Y);
	Z -= floor(Z);
	float u = Fade(X);
	float v = Fade(Y);
	float w = Fade(Z);
	int A = int(p[a] + b) & 0xff;
	int B = int(p[a + 1] + b) & 0xff;
	int AA = int(p[A] + c) & 0xff;
	int BA = int(p[B] + c) & 0xff;
	int AB = int(p[A + 1] + c) & 0xff;
	int BB = int(p[B + 1] + c) & 0xff;
	return Interpolate(Interpolate(Interpolate(Gradient(p[AA], X, Y, Z), Gradient(p[BA], X - 1, Y, Z), u),
		Interpolate(Gradient(p[AB], X, Y - 1, Z), Gradient(p[BB], X - 1, Y - 1, Z), u), v),
		Interpolate(Interpolate(Gradient(p[AA + 1], X, Y, Z - 1), Gradient(p[BA + 1], X - 1, Y, Z - 1), u),
			Interpolate(Gradient(p[AB + 1], X, Y - 1, Z - 1), Gradient(p[BB + 1], X - 1, Y - 1, Z - 1), u), v), w) * amplitude;
}

float Interpolate(float a, float b, float x) {
	float t = x * PI;
	float f = (1 - cos(t)) * 0.5;
	return a * (1 - f) + b * f;
}

float Fade(float t) {
	return t * t * t * (t * (t * 6 - 15) + 10);
}

float Gradient(int hash, float X, float Y, float Z) {
	int h = hash & 15;
	float u = h < 8 ? X : Y;
	float v = h < 4 ? Y : (h == 12 || h == 14 ? X : Z);
	return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}