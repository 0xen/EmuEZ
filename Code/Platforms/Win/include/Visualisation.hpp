#pragma once


struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;


class Visualisation
{
public:

	enum class EVisualisationMode
	{
		ImGUI,
		Windowed
	};

	Visualisation( EVisualisationMode mode, unsigned int windowWidth, unsigned int windowHeight, unsigned int viewportWidth, unsigned int viewportHeight);

	~Visualisation();

	void SetupWindow();

	void DestroyWindow();

	void Update();

	void SetPixels( char* data, unsigned int size );

	void ClearScreen();

	EVisualisationMode GetMode();

	void SetMode( EVisualisationMode mode );
private:

	EVisualisationMode mMode;

	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Texture* screen_texture;

	unsigned int mWindowWidth;
	unsigned int mWindowHeight;
	unsigned int mViewportWidth;
	unsigned int mViewportHeight;
};