#pragma once


class Visualisation
{
public:


	Visualisation( unsigned int windowWidth, unsigned int windowHeight);

	~Visualisation();

	void SetPixels( char* data, unsigned int size );

	void ClearScreen();
private:
	unsigned int mWindowWidth;
	unsigned int mWindowHeight;
};