#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <X11/X.h>
#include <X11/keysym.h>
#include <mlx.h>

#define MAX_ITERATIONS 2048
#define WINDOW_WIDTH 1600
#define WINDOW_HEIGHT 1000

#define MLX_ERROR 1

typedef struct s_pixel
{
	double real;
	double imag;
	double iterations;
}	t_pixel;

typedef struct s_img
{
	long	zoom;
	void	*mlx_img;
	char	*addr;
	int		bpp;
	int		line_len;
	int		endian;
}	t_img;

typedef struct s_data
{
	int 	set;
	void	*mlx_ptr;
	void	*win_ptr;
	t_img	img;
	t_pixel mouse_pos;
}	t_data;

int		encode_rgb(u_int8_t red, u_int8_t green, u_int8_t blue)
{
	return (red << 16 | green << 8 | blue);
}

void	img_pix_put(t_img *img, int x, int y, int color)
{
	char    *pixel;
	int		i;

	i = img->bpp - 8;
	pixel = img->addr + (y * img->line_len + x * (img->bpp / 8));
	while (i >= 0)
	{
		/* big endian, MSB is the leftmost bit */
		if (img->endian != 0)
			*pixel++ = (color >> i) & 0xFF;
			/* little endian, LSB is the leftmost bit */
		else
			*pixel++ = (color >> (img->bpp - 8 - i)) & 0xFF;
		i -= 8;
	}
}

int color(t_pixel z){
	if (z.iterations >= MAX_ITERATIONS)
		return 0;
	double smoothed = log((log(z.real * z.real + z.imag * z.imag) / 2) / log(2)) / log(2);
	int color = (int)(sqrt(z.iterations - smoothed) * 256) % 2048;
	if(color < 328)
		return(encode_rgb(0 + color * (double)(32 - 0) / 328,
						  7 + color * (double)(107 - 7) / 328,
						  100 + color * (double)(203 - 100) / 328));
	else if(color < 860)
		return(encode_rgb(32 + (color - 328) * (double)(237 - 32) / 532,
						  107 + (color - 328) * (double)(255 - 107) / 532,
						  203 + (color - 328) * (double)(255 - 203) / 532));
	else if(color < 1315)
		return(encode_rgb(237 + (color - 860) * (double)(255 - 237) / 455,
						  255 - (color - 860) * (double)(255 - 170) / 455,
						  255 - (color - 860) * (double)(255 - 0) / 455));
	else if(color < 1756)
		return(encode_rgb(255 - (color - 1315) * (double)(255 - 0) / 441,
						  170 - (color - 1315) * (double)(170 - 2) / 441,
						  0 + (color - 1315) * (double)(0 - 0) / 441));
	else
		return(encode_rgb(0 + (color - 1756) * (double)(0 - 0) / 292,
						  2 + (color - 1756) * (double)(7 - 2) / 292,
						  0 + (color - 1756) * (double)(100 - 0) / 292));
}

t_pixel mandelbrot_iterations(t_pixel c)
{
	t_pixel z;
	double zx2, zy2;

	z.iterations = 0;
	z.real = 0.0;
	z.imag = 0.0;
	zx2 = 0.0;
	zy2 = 0.0;
	while (z.iterations < MAX_ITERATIONS && zx2 + zy2 <= 4) {
		z.imag = 2 * z.real * z.imag + c.imag;
		z.real = zx2 - zy2 + c.real;
		zx2 = z.real * z.real;
		zy2 = z.imag * z.imag;
		z.iterations++;
	}
	return z;
}

t_pixel julia_iterations(t_pixel c, t_pixel z)
{
	double zx2, zy2;

	z.iterations = 0;
	zx2 = z.real * z.real;
	zy2 = z.imag * z.imag;
	while (z.iterations < MAX_ITERATIONS && zx2 + zy2 <= 4) {
		z.imag = z.real * z.imag * 2 + c.imag;
		z.real = zx2 - zy2 + c.real;
		zx2 = z.real * z.real;
		zy2 = z.imag * z.imag;
		z.iterations++;
	}
	return z;
}

void	render_set(t_img *img, t_data *data, int set)
{
	t_pixel c;

	c.imag = 0;
	while (c.imag < WINDOW_HEIGHT)
	{
		c.real = 0;
		while (c.real < WINDOW_WIDTH)
		{
			if (set == 1)
				img_pix_put(img, c.real, c.imag,
							color(julia_iterations((t_pixel){0.285, 0.01},
												   (t_pixel){(((double)c.real - data->mouse_pos.real - WINDOW_WIDTH / 2) / img->zoom),
															 (((double)c.imag - data->mouse_pos.imag - WINDOW_HEIGHT / 2) / img->zoom),
															 0})));
			else
				img_pix_put(img, c.real, c.imag,
							color(mandelbrot_iterations((t_pixel){(((double)c.real - WINDOW_WIDTH / 2) / img->zoom),
																  (((double)c.imag - WINDOW_HEIGHT / 2) / img->zoom),
																  0})));
			++c.real;
		}
		++c.imag;
	}
}

int	render(t_data *data)
{
	if (data->win_ptr == NULL)
		return (1);
	render_set(&data->img, data, 1);
	mlx_put_image_to_window(data->mlx_ptr, data->win_ptr, data->img.mlx_img, 0, 0);

	return (0);
}

int	handle_keypress(int keysym, t_data *data)
{
	if (keysym == XK_Escape)
	{
		mlx_destroy_window(data->mlx_ptr, data->win_ptr);
		data->win_ptr = NULL;
	}
	if (keysym == XK_g)
		data->img.zoom *= 1.5;
	printf("%d\n", data->img.zoom);
	return (0);
}

int handle_scroll(int keysym, t_data *data)
{
	printf("test\n\ntest\n\n%d\n", data->img.zoom);
	if (keysym == 4)
		data->img.zoom *= 1.5;
	if (keysym == 5)
		data->img.zoom *= 1.5;
	return (0);
}

int	main(void)
{
	t_data	data;

	data.mlx_ptr = mlx_init();
	if (data.mlx_ptr == NULL)
		return (MLX_ERROR);
	data.win_ptr = mlx_new_window(data.mlx_ptr, WINDOW_WIDTH, WINDOW_HEIGHT, "my window");
	if (data.win_ptr == NULL)
	{
		free(data.win_ptr);
		return (MLX_ERROR);
	}
	data.img.mlx_img = mlx_new_image(data.mlx_ptr, WINDOW_WIDTH, WINDOW_HEIGHT);
	data.img.addr = mlx_get_data_addr(data.img.mlx_img, &data.img.bpp,
									  &data.img.line_len, &data.img.endian);
	data.img.zoom = 200;
	mlx_loop_hook(data.mlx_ptr, &render, &data);
	mlx_hook(data.win_ptr, KeyPress, KeyPressMask, &handle_keypress, &data);
	mlx_hook(data.win_ptr, ButtonPress, ButtonPressMask, &handle_scroll, &data);
	mlx_loop(data.mlx_ptr);
	mlx_destroy_image(data.mlx_ptr, data.img.mlx_img);
	mlx_destroy_display(data.mlx_ptr);
	free(data.mlx_ptr);
	return(0);
}