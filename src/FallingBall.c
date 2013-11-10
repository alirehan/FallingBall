#include "pebble.h"
#include <stdio.h>

#define MATH_PI 3.141592653589793238462
#define NUM_DISCS 1
#define NUM_FLOORS 7
#define DISC_DENSITY .1
#define ACCEL_RATIO 0.02
#define GRAVITY 5
#define RADIUS 5
#define FLOOR_SPACE 25
#define HOLE_SIZE 25
#define INIT_SPEED 1
#define INCREMENT 0.01
#define DAMPING 0.5


typedef struct Vec2d {
  double x;
  double y;
} Vec2d;

typedef struct Disc {
  Vec2d pos;
  Vec2d vel;
  double mass;
  double radius;
} Disc;

typedef struct Floor {
  double y;
  double hole_x;
  double hole_size;  
} Floor;


static Disc discs[NUM_DISCS];
static Floor floors[NUM_FLOORS];

static Window *window;
static GRect window_frame;
static Layer *game_layer;
static AppTimer *timer;
static int nextFloor = 20;
static int score = 0;
static bool game_over = false;
static int LOOP_TIME = 30;
static int speed = INIT_SPEED;

static void init(void);
static void deinit(void);

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  window_stack_pop(true);
  deinit();
  init();
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
}


static double disc_calc_mass(Disc *disc) {
  return MATH_PI * disc->radius * disc->radius * DISC_DENSITY;
}

static void disc_init(Disc *disc) {
  disc->pos.x = window_frame.size.w/2;
  disc->pos.y = window_frame.size.h/2;
  disc->vel.x = 0;
  disc->vel.y = 0;
  disc->radius = RADIUS;
  disc->mass = disc_calc_mass(disc);
}

static void floor_init(Floor *floor) {
  floor->y = nextFloor;
  floor->hole_x = rand() % (window_frame.size.w - 3 * RADIUS);
  floor->hole_size = HOLE_SIZE ;
  nextFloor += FLOOR_SPACE;
}

static void floor_reinit(Floor *floor) {
  floor->y = window_frame.size.h+FLOOR_SPACE;
  floor->hole_x = rand() % (window_frame.size.w - 3 * RADIUS);
  floor->hole_size = HOLE_SIZE ;
}

static void disc_apply_force(Disc *disc, Vec2d force) {
  disc->vel.x += force.x / disc->mass;
  disc->vel.y = force.y / disc->mass;
}

static void disc_apply_accel(Disc *disc, AccelData accel) {
  Vec2d force;
  force.x = accel.x * ACCEL_RATIO;
  force.y = GRAVITY;
  disc_apply_force(disc, force);
}


static void disc_update(Disc *disc) {
  const GRect frame = window_frame;

  if ((disc->pos.x - disc->radius < 0 && disc->vel.x < 0)
    || (disc->pos.x + disc->radius > frame.size.w && disc->vel.x > 0)) {
    disc->vel.x = -disc->vel.x * DAMPING ;
  }
  disc->pos.x += disc->vel.x;
  
  
  for (int i = 0; i < NUM_FLOORS; i++) {
    if ((disc->pos.y - disc->radius <= ((Floor)floors[i]).y 
    		&& disc->pos.y + disc->radius >= ((Floor)floors[i]).y )
    		&& (disc->pos.x+disc->radius  <= ((Floor)floors[i]).hole_x 
    		|| disc->pos.x+disc->radius  >= ((Floor)floors[i]).hole_x + ((Floor)floors[i]).hole_size))     
    {
  			disc->pos.y = ((Floor)floors[i]).y - disc->radius;
  	}
  	else
  	{
  		disc->pos.y += disc->vel.y;
  	}
	}
	
	if(disc->pos.y - disc->radius < 0 || disc->pos.y + disc->radius > frame.size.h)
  {
  	game_over = true;
  }
  

	
	
}

static void floor_update(Floor *floor) {
  floor->y -= speed;
	if (floor->y < 0)
	{
		score += 1;
		if(score%10==0)
			LOOP_TIME -=0.01;
		floor_reinit(floor);
	}
			
}

static void disc_draw(GContext *ctx, Disc *disc) {
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, GPoint(disc->pos.x, disc->pos.y), disc->radius);
}

static void floor_draw(GContext *ctx, Floor *floor) {
  graphics_context_set_stroke_color(ctx, GColorWhite);
  
	graphics_draw_line	(ctx,GPoint(0, floor->y),GPoint(floor->hole_x, floor->y));	
	graphics_draw_line	(ctx,GPoint(floor->hole_x+floor->hole_size, floor->y),GPoint(window_frame.size.w, floor->y));	
}


static void game_layer_update_callback(Layer *me, GContext *ctx) {
  
	/*  
  for (int i = 0; i < NUM_DISCS; i++) {
    disc_draw(ctx, &discs[i]);
  }
  */
  
  
  if (game_over)
  {
	  static char scoreCard[50];
  	snprintf(scoreCard,sizeof(scoreCard),"Game Over! \n YourScore: %d ",score);

	
  	graphics_draw_text(ctx,  
             scoreCard,  
             fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21),  
             GRect(0, 30, window_frame.size.w, window_frame.size.h),  
             GTextOverflowModeWordWrap,  
             GTextAlignmentCenter,  
             NULL);  
  	return;
  }
  
	for (int i = 0; i < NUM_FLOORS; i++) {
    floor_draw(ctx, &floors[i]);
  }
  
  disc_draw(ctx, &discs[0]);
  
  static char scoreDisplay[20];
  snprintf(scoreDisplay,sizeof(scoreDisplay),"Score: %d Time: %d",score,LOOP_TIME );

	
  graphics_draw_text(ctx,  
             scoreDisplay,  
             fonts_get_system_font(FONT_KEY_GOTHIC_14),  
             GRect(0, 30, window_frame.size.w, window_frame.size.h),
             GTextOverflowModeWordWrap,  
             GTextAlignmentCenter,  
             NULL);    
}

static void timer_callback(void *data) {


  AccelData accel = { 0, 0, 0 };
  accel_service_peek(&accel);

	if (game_over){
		if(abs(accel.x) + abs(accel.y) + abs(accel.z) > 2000)
		{	
			window_stack_pop(true);
  		deinit();
  		init();
  		return;
  	}
  }
	else{
  	for (int i = 0; i < NUM_FLOORS; i++) {
     Floor *floor = &floors[i];
     floor_update(floor);
  	}
  
  	Disc *disc = &discs[0];
  	disc_apply_accel(disc, accel);
  	disc_update(disc);
	
  	/* 
  	for (int i = 0; i < NUM_DISCS; i++) {
     Disc *disc = &discs[i];
     disc_apply_accel(disc, accel);
     disc_update(disc);
  	}
  	*/

  	layer_mark_dirty(game_layer);

  }
  timer = app_timer_register(LOOP_TIME /* milliseconds */, timer_callback, NULL);
}

static void handle_accel(AccelData *accel_data, uint32_t num_samples) {

}

static void window_load(Window *window) {
  srand(time(NULL));  // Seed ONCE
	
	Layer *window_layer = window_get_root_layer(window);
  GRect frame = window_frame = layer_get_frame(window_layer);

  game_layer = layer_create(frame);
  layer_set_update_proc(game_layer, game_layer_update_callback);
  layer_add_child(window_layer, game_layer);
 
 	for (int i = 0; i < NUM_FLOORS; i++) {
  	floor_init(&floors[i]);
  }
 
  disc_init(&discs[0]);
  


  /*
  for (int i = 0; i < NUM_DISCS; i++) {
  	disc_init(&discs[i]);
  }

  text_layer = text_layer_create(GRect(4, 44, frame.size.w, 60));
  text_layer_set_background_color(text_layer, GColorClear);
  text_layer_set_text_color(text_layer, GColorWhite);
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  text_layer_set_text(text_layer, "Alarm system!");
  layer_add_child(window_layer, text_layer_get_layer(text_layer));
	*/


}

static void window_unload(Window *window) {
  layer_destroy(game_layer);
}

static void init(void) {
 nextFloor = 20;
 score = 0;
 game_over = false;
 LOOP_TIME = 30;
 speed = INIT_SPEED;


  window = window_create();
  window_set_click_config_provider(window, click_config_provider);
  
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload
  });
  window_stack_push(window, true /* Animated */);
  window_set_background_color(window, GColorBlack);

  accel_data_service_subscribe(0, handle_accel);

  timer = app_timer_register(LOOP_TIME /* milliseconds */, timer_callback, NULL);
}

static void deinit(void) {
  accel_data_service_unsubscribe();
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
