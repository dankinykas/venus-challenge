// TU/e Engineering challenge for venus
// This is the robot code main file
#include <libpynq.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <time.h>
#include <stepper.h>
#include <communications.h>
#include <edge_sensors.h>
#include <tcs3472.h> // color sensor library
#include <vl53l0x.h> // distance sensor library

#define TOF_ADDRESS_LOW 0x69
#define TOF_ADDRESS_HIGH 0x70
#define TOF_ADDRESS_MIDDLE 0x71

#define POWER_PIN_COLOR_SENSOR IO_AR10
#define POWER_PIN_DISTANCE_SENSOR_LOWER IO_AR9
#define POWER_PIN_DISTANCE_SENSOR_MIDDLE IO_AR11
#define POWER_PIN_DISTANCE_SENSOR_UPPER IO_AR12

#define INTEGRATION_TIME_MS 60

//from algorithm
#define TURN 400
#define RED 31
#define BLUE 34
#define GREEN 32
#define WHITE 37
#define WHITEBACK 47
#define BLACK 047 // 30;47
#define YELLOWBACK 43 // 30;43
#define YELLOW 33
#define GRAY 30
#define BROWN 94
#define BLOCKSIZE 1

typedef struct {
  char status;
  char colour;
  int row;
  int col;
  char prevStatus;
  int prevColour;
} GridCell;
//

// Code to power up color sensor, adapted from tcs3472 library example code
int color_sensor_init(tcs3472* color_sensor);

// Code to power up distance sensors, adapted from vl53l0x library example code
int distance_sensor_init(vl53x* distance_sensor_lower, vl53x* distance_sensor_upper, vl53x* distance_sensor_middle);

void print_colour(uint16_t red, uint16_t green, uint16_t blue)
{
    printf("\033[3F\033[0J"); // Move cursor back 3 lines
    printf("\033[48;2;%hhu;%hhu;%hhum      \033[0m", CLAMP_255((red >> 4)), CLAMP_255((green >> 4)), CLAMP_255((blue >> 4))); // print uint8_t's
    printf("R: %hhu\n", CLAMP_255((red >> 4))); // print uint16_t
    printf("\033[48;2;%hhu;%hhu;%hhum      \033[0m", CLAMP_255((red >> 4)), CLAMP_255((green >> 4)), CLAMP_255((blue >> 4))); // print uint8_t's
    printf("G: %hu\n", CLAMP_255((green >> 4))); // print uint16_t
    printf("\033[48;2;%hhu;%hhu;%hhum      \033[0m", CLAMP_255((red >> 4)), CLAMP_255((green >> 4)), CLAMP_255((blue >> 4))); // print uint8_t's
    printf("B: %hu\n",  CLAMP_255((blue >> 4))); // print uint16_t
    fflush(NULL);
}


////algorithm
// Function to create the initial grid
GridCell** createGrid(int rows, int cols) {
  GridCell** grid = (GridCell**)malloc(rows * sizeof(GridCell*)); // Allocate memory for rows

  for (int i = 0; i < rows; i++) {
    grid[i] = (GridCell*)malloc(cols * sizeof(GridCell)); // Allocate memory for each row's columns
  }

  return grid;
}


// Function to expand the grid
GridCell** expandGrid(GridCell** grid, int newRows, int newCols, int initialRows, int initialCols){

  if(newRows > initialRows){
    grid = (GridCell**)realloc(grid, newRows * sizeof(GridCell*));
  }

  for (int i = 0; i < newRows; i++) {
    if (i >= initialRows) {
      grid[i] = (GridCell*)malloc(newCols * sizeof(GridCell)); // Allocate memory for new rows' columns
    } else if(newCols > initialCols){
      grid[i] = (GridCell*)realloc(grid[i], newCols * sizeof(GridCell)); // Reallocate memory for existing rows' columns
    }
  }
  return grid;
}


// Function to print the grid
void printGrid(GridCell** grid, int rows, int cols) {
  
  for (int i = 0; i < rows+1; i++) {
    for (int j = 0; j < cols+1; j++) {
      if(i == 0){
        if(j == 0){
          printf("    ");
        } else {
          printf("\033[4m%3d\033[0m", grid[i][j-1].col); // print col numbers
        }
      } else if(j == 0){
        printf("%4d|", grid[i-1][j].row); // print row numbers
      } else if(grid[i-1][j-1].prevStatus == 'S'){
        printf(" \033[32mS\033[0m ");
      } else {
        switch (grid[i-1][j-1].colour){ // print grid
        case RED:         printf(" \033[31m%c\033[0m ", grid[i-1][j-1].status);        break;
        case BLUE:        printf(" \033[34m%c\033[0m ", grid[i-1][j-1].status);        break;
        case GREEN:       printf(" \033[32m%c\033[0m ", grid[i-1][j-1].status);        break;
        case WHITE:       printf(" \033[37m%c\033[0m ", grid[i-1][j-1].status);        break;
        case WHITEBACK:   printf("\033[47m%3c\033[0m", grid[i-1][j-1].status);         break;
        case BLACK:       printf(" \033[30;47m%c\033[0m ",grid[i-1][j-1].status);      break;
        case YELLOWBACK:  printf("\033[1;30;43m%3c\033[0m",grid[i-1][j-1].status);    break;
        case YELLOW:      printf(" \033[1;33m%c\033[0m ", grid[i-1][j-1].status);      break;
        case GRAY:        printf(" \033[30m%c\033[0m ", grid[i-1][j-1].status);        break;
        case BROWN:       printf(" \033[38;5;94m%c\033[0m ", grid[i-1][j-1].status);   break;
        default:          printf(" %c ", grid[i-1][j-1].status);                       break;
        }
      }
    }
    printf("\n");
  }
  printf("\n");
}


// Function to fill the initial grid
void initialiseGrid(GridCell** grid, int rows, int cols, int pRows, int pCols){

  int midRow = rows/2;
  int midCol = cols/2;
  if(midRow/2*2 == midRow){
    midRow--;
  }
  if(midCol/2*2 == midCol){
    midCol--;
  }

  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < cols; j++) {
      if(i < pRows && j < pCols && (*grid)->status == '\0'){
        j = pCols;
      }
      grid[i][j].row = i-midRow;
      grid[i][j].col = j-midCol;
      if(grid[i][j].row == 0 && grid[i][j].col == 0){
        grid[i][j].status = 'S';
        grid[i][j].colour = GREEN;
        grid[i][j].prevStatus = 'S';
        grid[i][j].prevColour = GREEN;
      } else {
        grid[i][j].status = '#';
        grid[i][j].colour = GRAY;
        grid[i][j].prevStatus = '#';
        grid[i][j].prevColour = GRAY;
      }
    }
  }
}


// Function to fill the expanded grid
void re_initialiseGrid(GridCell** grid, int rows, int cols, int dirRow, int dirCol){

  int pRows = rows - abs(dirRow);
  int pCols = cols - abs(dirCol);
  int tmp = -1;


  for (int i = 0; i < rows; i++){
    if(dirRow > 0 && dirCol == 0){ ////skips to newly created rows
      i = pRows;
    }
    for (int j = 0; j < cols; j++){
      if(i < pRows && j < pCols && dirCol > 0){ ////skips to newly created columns
        j = pCols;
      }

      if(dirCol > 0 && j >= pCols){ ////set column index for new column positive
        grid[i][j].col = grid[i][j-1].col + dirCol;
        grid[i][j].row = grid[i][j-1].row;
        grid[i][j].status = '#';
        grid[i][j].colour = GRAY;
        grid[i][j].prevStatus = '#';
        grid[i][j].prevColour = GRAY;

      } else if(dirCol < 0){ ////set column index for new column negative
        if(j == 0){
          grid[i][j].col = grid[i][j].col + dirCol;
        } else {
          grid[i][j].col = grid[i][j-1].col+1;
        }
        if(j >= pCols){
          grid[i][j].row = grid[i][j-1].row;
        }

        for (int h = cols-1; h >= 0; h--){
          if(tmp < i){
            if(h == 0){
              /*if(grid[i][h+1].status == '^' || grid[i][h+1].status == '>' || grid[i][h+1].status == 'v' || grid[i][h+1].status == '<'){
                grid[i][h].status = grid[i][h+1].status;
                grid[i][h].colour = YELLOW;
              } else {*/
                grid[i][h].status = '#';
                grid[i][h].colour = GRAY;
                grid[i][h].prevStatus = '#';
                grid[i][h].prevColour = GRAY;
              //}
            } else {
              grid[i][h].status = grid[i][h-1].status;
              grid[i][h].colour = grid[i][h-1].colour;
              grid[i][h].prevStatus = grid[i][h-1].prevStatus;
              grid[i][h].prevColour = grid[i][h-1].prevColour;
            }
          }
        }
        tmp = i;
      }

      

      if(dirRow > 0 && i >= pRows){ ////set row index for new row positive
        grid[i][j].row = grid[i-1][j].row + dirRow;
        grid[i][j].col = grid[i-1][j].col;
        grid[i][j].status = '#';
        grid[i][j].colour = GRAY;
        grid[i][j].prevStatus = '#';
        grid[i][j].prevColour = GRAY;

      } else if(dirRow < 0){ ////set row index for new row negative
        if(i == 0){
          grid[i][j].row = grid[i][j].row + dirRow;
        } else {
          grid[i][j].row = grid[i-1][j].row+1;
        }
        if(i >= pRows){
          grid[i][j].col = grid[i-1][j].col;
        }

        for (int h = rows-1; h >= 0; h--){
          if(tmp < j){
            if(h == 0){
              /*if(grid[h+1][j].status == '^' || grid[h+1][j].status == '>' || grid[h+1][j].status == 'v' || grid[h+1][j].status == '<'){
                grid[h][j].status = grid[h+1][j].status;
                grid[h][j].colour = YELLOW;
              } else {*/
                grid[h][j].status = '#';
                grid[h][j].colour = GRAY;
                grid[h][j].prevStatus = '#';
                grid[h][j].prevColour = GRAY;
              //}
            } else {
              grid[h][j].status = grid[h-1][j].status;
              grid[h][j].colour = grid[h-1][j].colour;
              grid[h][j].prevStatus = grid[h-1][j].prevStatus;
              grid[h][j].prevColour = grid[h-1][j].prevColour;
            }
          }
        }
        if(j >= cols-1){
          tmp = j;
        }
      }
    }
  }

}


// Function to free memory allocated for the grid
void freeGrid(GridCell** grid, int rows) {
  for (int i = 0; i < rows; i++) {
    if(grid[i] != NULL){
      free(grid[i]); // Free memory for each row's columns
    }
  }
  
  if(grid != NULL){
    free(grid); // Free memory for rows
  }
}


void stepperMotor(int curDir, int prevDir){

  int turn = 0;
  int16_t left = 0;
  int16_t right = 0;
  printf("%d, %d\n", curDir, prevDir);


  if(prevDir != curDir){
    for (int i = prevDir; i < 5; i++){
      turn++;
      if(i > 3){
        i = 0;
      }
      if(i == curDir){
        break;
      }
    }
	printf("turn: %d\n", turn);
    if(turn == 4){
      turn = -2;
    }
	if(turn == 3){
		stepper_steps(-300, -300);
		while(!stepper_steps_done()){
	      stepper_get_steps(&left, &right);
		  //printf("left, right: %d, %d\n", left, right);
  		}
	} else {
    	stepper_steps(-300*turn, 300*turn);
		while(!stepper_steps_done()){
		stepper_get_steps(&left, &right);
		//printf("left, right: %d, %d\n", left, right);
  }
	}
  } else {

  stepper_steps(100, 100);
  }
  while(!stepper_steps_done()){
	stepper_get_steps(&left, &right);
	//printf("left, right: %d, %d\n", left, right);
  }

} 


// Function to make bigger robot
void printRobot(GridCell** grid, int midRow, int midCol, char direction, int maxRow, int maxCol){

  //printf("%d, %d\n", midRow, midCol);

  int arrow_i = -1;
  int arrow_j = -1;
  int rows = 0, cols = 0;

  for (int i = 0; i < maxRow; i++){
    for (int j = 0; j < maxCol; j++){
      if(grid[i][j].row == midRow && grid[i][j].col == midCol){
        rows = i;
        cols = j;
      }
    }
  }


  // Adjust arrow position, grid size, and orientation based on the direction
  switch (direction) {
    case '<':
    arrow_j = cols-3;
    break;
    case '^':
    arrow_i = rows-3;
    break;
    case 'v':
    arrow_i = rows+3;
    break;
    case '>':
    arrow_j = cols+3;
    break;
    default:
    break;
  }


  for (int i = rows-3; i < rows + 4; i++){
    for (int j = cols-3; j < cols + 4; j++){
      //printf("i, j: %d, %d\n", i, j);
      if(j == arrow_j && i > rows-2 && i < rows+2){
        //printf(" \033[1;33m%c\033[0m ", direction);
        grid[i][j].status = direction;
        grid[i][j].colour = YELLOW;
      } else if (i == arrow_i && j > cols-2 && j < cols+2){
        //printf(" \033[1;33m%c\033[0m ", direction);
        grid[i][j].status = direction;
        grid[i][j].colour = YELLOW;
      } else if(i == rows && j == cols){
        //printf(" \033[1;33mR\033[0m ");
        grid[i][j].status = 'R';
        grid[i][j].colour = YELLOW;
      } else if(grid[i][j].status == '#' || grid[i][j].status == '*' || grid[i][j].colour == YELLOW){
        //printf("\033[43m   \033[0m");
        grid[i][j].status = ' ';
        grid[i][j].colour = YELLOWBACK;
      }
    }
    //printf("\n");
  }
}


// Function to move inside the grid
GridCell** algorithmMovement(GridCell** grid, int* maxRow, int* maxCol, vl53x* distance_sensor_lower, vl53x* distance_sensor_middle, vl53x* distance_sensor_upper, tcs3472* color_sensor/*, GridCell** map, int mapRow, int mapCol, */){

  int row = 0;
  int col = 0;
  int minRow = grid[0][0].row;
  int minCol = grid[0][0].col;
  int rowMax = grid[*maxRow-1][*maxCol-1].row;
  int colMax = grid[*maxRow-1][*maxCol-1].col;
  int direction = 0;
  int prevDir = 0;
  int tmpDir = 0;
  char status;
  int colour = 0;

  int tmpH = 0;
  int tmpK = 0;

  uint32_t iDistanceLow;
  uint32_t iDistanceMid;
  uint32_t iDistanceHigh;

    tcsReading rgb;
	printf("        \n        \n        \n"); // Buffer some space
	//for (int i = 0; i < 12; i++)
	//{
		//i = 
		tcs_get_reading(color_sensor, &rgb);
		print_colour(rgb.red, rgb.green, rgb.blue); //Used to print colour to screen
		sleep_msec(INTEGRATION_TIME_MS + 20);
	//}



  //search starting/continuation point
  for (int i = 0; i < *maxRow; i++){
    for (int j = 0; j < *maxCol; j++){
      if(grid[i][j].status == 'R'){
        for (int h = 0; h < *maxRow; h++){
          for (int k = 0; k < *maxCol; k++){
            if(grid[h][k].colour == YELLOWBACK){
              grid[h][k].status = grid[h][k].prevStatus;
              grid[h][k].colour = grid[h][k].prevColour;
            }
          }
        }
        row = grid[i][j].row;
        col = grid[i][j].col;
        printRobot(grid, row, col, '^', *maxRow, *maxCol);
        break;
      }
      if(grid[i][j].status == 'S' && row == 0 && col == 0){
        row = grid[i][j].row;
        col = grid[i][j].col;
        printRobot(grid, row, col, '^', *maxRow, *maxCol);
      }
    }
  }


  do{


    //expand grid to accommodate the to be checked area
    while(row - 8 < minRow){
      grid = expandGrid(grid, *maxRow+1, *maxCol, *maxRow, *maxCol);
      (*maxRow)++;
      re_initialiseGrid(grid, *maxRow, *maxCol, -1, 0);
      minRow--;
    }

    while(col + 8 > colMax){
      grid = expandGrid(grid, *maxRow, *maxCol+1, *maxRow, *maxCol);
      (*maxCol)++;
      re_initialiseGrid(grid, *maxRow, *maxCol, 0, 1);
      colMax++;
    }

    while(row + 8 > rowMax){
      grid = expandGrid(grid, *maxRow+1, *maxCol, *maxRow, *maxCol);
      (*maxRow)++;
      re_initialiseGrid(grid, *maxRow, *maxCol, 1, 0);
      rowMax++;
    }

    while(col - 8 < minCol){
      grid = expandGrid(grid, *maxRow, *maxCol+1, *maxRow, *maxCol);
      (*maxCol)++;
      re_initialiseGrid(grid, *maxRow, *maxCol, 0, -1);
      minCol--;;
    }
    

    //printf("row, col: %d, %d\n", row, col);
    /*if(row < 0 && col <= 1){
      direction = 1;
    } else if(col > 0){
      direction = 2;
    }
    if(row > 0){
      break;
    }*/
	/*if(row == -1){
	  break;
	}*/

	//distance sensor
	iDistanceLow = tofReadDistance(distance_sensor_lower);
	iDistanceMid = tofReadDistance(distance_sensor_middle);
	iDistanceHigh = tofReadDistance(distance_sensor_upper);

	if(iDistanceLow < 30 || iDistanceMid < 110 || iDistanceHigh < 130){
	  if(iDistanceHigh < 130){
		status = 'M';
		colour = BROWN;
	  } else if(iDistanceMid < 110){
		status = '6';
		//check colour
	  } else if(iDistanceLow < 30){
		status = '3';
		//check colour
	  }
		for(int i = 0; i < *maxRow; i++){
			for(int j = 0; j < *maxCol; j++){
				if(grid[i][j].row == row && grid[i][j].col == col){
					switch(direction){
					case 0:
					grid[i-4][j].status = status;
					grid[i-4][j].colour = colour;
					grid[i-4][j].prevStatus = status;
					grid[i-4][j].prevColour = colour;
					break;
					case 1:
					grid[i][j+4].status = status;
					grid[i][j+4].colour = colour;
					grid[i][j+4].prevStatus = status;
					grid[i][j+4].prevColour = colour;
					break;
					case 2:
					grid[i+4][j].status = status;
					grid[i+4][j].colour = colour;
					grid[i+4][j].prevStatus = status;
					grid[i+4][j].prevColour = colour;
					break;
					case 3:
					grid[i][j-4].status = status;
					grid[i][j-4].colour = colour;
					grid[i][j-4].prevStatus = status;
					grid[i][j-4].prevColour = colour;
					break;
					}
				}
			}
		}
	  

		direction++;
	}
	if(col > 5 || row < -5 || col < -5 || row > 5){
		break;
	}
	
	//edge sensor
	if(get_edge(LEFT)){
		printf("edge\n");
		for(int i = 0; i < *maxRow; i++){
			for(int j = 0; j < *maxCol; j++){
				if(grid[i][j].row == row && grid[i][j].col == col){
					switch(direction){
					case 0:
					grid[i-3][j-3].prevStatus = ' ';
					grid[i-3][j-3].prevColour = WHITEBACK;
					break;
					case 1:
					grid[i-3][j+3].prevStatus = ' ';
					grid[i-3][j+3].prevColour = WHITEBACK;
					break;
					case 2:
					grid[i+3][j+3].prevStatus = ' ';
					grid[i+3][j+3].prevColour = WHITEBACK;
					break;
					case 3:
					grid[i+3][j-3].prevStatus = ' ';
					grid[i+3][j-3].prevColour = WHITEBACK;
					break;
					}
				}
			}
		}
		direction += 2;
		tmpDir++;
	}
	if(get_edge(RIGHT)){
		printf("edge\n");
		for(int i = 0; i < *maxRow; i++){
			for(int j = 0; j < *maxCol; j++){
				if(grid[i][j].row == row && grid[i][j].col == col){
					switch(direction){
					case 0:
					grid[i-3][j+3].prevStatus = ' ';
					grid[i-3][j+3].prevColour = WHITEBACK;
					break;
					case 1:
					grid[i+3][j+3].prevStatus = ' ';
					grid[i+3][j+3].prevColour = WHITEBACK;
					break;
					case 2:
					grid[i+3][j-3].prevStatus = ' ';
					grid[i+3][j-3].prevColour = WHITEBACK;
					break;
					case 3:
					grid[i-3][j-3].prevStatus = ' ';
					grid[i-3][j-3].prevColour = WHITEBACK;
					break;
					}
				}
			}
		}
		direction += 2;
		tmpDir++;
	}

	if(direction > 3){
			direction -= 4;
	}



	stepperMotor(direction, prevDir);


    if(prevDir != direction){

		if(tmpDir > 0){
			direction--;
			if(direction < 0){
				direction += 4;
			}
			stepperMotor(direction, prevDir);
			tmpDir = 0;

			for (int i = 0; i < *maxRow; i++){
				for (int j = 0; j < *maxCol; j++){
				if( grid[i][j].status == 'R'){
					tmpH = i - 3;
					tmpK = j - 3;

					for (int h = tmpH; h < tmpH + 7; h++){
					for (int k = tmpK; k < tmpK + 7; k++){

						if(grid[h][k].prevStatus == '#'){
						grid[h][k].status = '*';
						grid[h][k].colour = 0;
						grid[h][k].prevStatus = '*';
						grid[h][k].prevColour = 0;
						} else {
						grid[h][k].status = grid[h][k].prevStatus;
						grid[h][k].colour = grid[h][k].prevColour;
						}
					}
					}
					
				}
				}
			}

			switch(prevDir){
			case 0:
			row++;
			break;
			case 1:
			col--;
			break;
			case 2:
			row--;
			break;
			case 3:
			col++;
			break;
			}
		} 
	  
		switch(direction){
			case 0: printRobot(grid, row, col, '^', *maxRow, *maxCol); break;
			case 1: printRobot(grid, row, col, '>', *maxRow, *maxCol); break;
			case 2: printRobot(grid, row, col, 'v', *maxRow, *maxCol); break;
			case 3: printRobot(grid, row, col, '<', *maxRow, *maxCol); break;
			default: break;
		}
		
    } else {

      for (int i = 0; i < *maxRow; i++){
        for (int j = 0; j < *maxCol; j++){
          if( grid[i][j].status == 'R'){
            tmpH = i - 3;
            tmpK = j - 3;

            for (int h = tmpH; h < tmpH + 7; h++){
              for (int k = tmpK; k < tmpK + 7; k++){

                if(grid[h][k].prevStatus == '#'){
                  grid[h][k].status = '*';
                  grid[h][k].colour = 0;
                  grid[h][k].prevStatus = '*';
                  grid[h][k].prevColour = 0;
                } else {
                  grid[h][k].status = grid[h][k].prevStatus;
                  grid[h][k].colour = grid[h][k].prevColour;
                }
              }
            }
            
          }
        }
      }

      switch (direction){
      case 0: /// up

      row--;
      if(row-3 < minRow){
        grid = expandGrid(grid, *maxRow+1, *maxCol, *maxRow, *maxCol);
        (*maxRow)++;
        re_initialiseGrid(grid, *maxRow, *maxCol, -1, 0);
        minRow--;
      }
      printf("\nup\n");
      break;
      case 1: /// right

      col++;
      if(col+3 > colMax){
        grid = expandGrid(grid, *maxRow, *maxCol+1, *maxRow, *maxCol);
        (*maxCol)++;
        re_initialiseGrid(grid, *maxRow, *maxCol, 0, 1);
        colMax++;
      }
      printf("\nright\n");
      break;
      case 2: /// down

      row++;
      if(row+3 > rowMax){
        grid = expandGrid(grid, *maxRow+1, *maxCol, *maxRow, *maxCol);
        (*maxRow)++;
        re_initialiseGrid(grid, *maxRow, *maxCol, 1, 0);
        rowMax++;
      }
      printf("\ndown\n");
      break;
      case 3: /// left

      col--;
      if(col-3 < minCol){
        grid = expandGrid(grid, *maxRow, *maxCol+1, *maxRow, *maxCol);
        (*maxCol)++;
        re_initialiseGrid(grid, *maxRow, *maxCol, 0, -1);
        minCol--;;
      }
      printf("\nleft\n");
      break;
      default:
      break;
      }

      for (int i = 0; i < *maxRow; i++){
        for (int j = 0; j < *maxCol; j++){
          if(grid[i][j].row == row && grid[i][j].col == col){

            switch(direction){
              case 0: printRobot(grid, row, col, '^', *maxRow, *maxCol); break;
              case 1: printRobot(grid, row, col, '>', *maxRow, *maxCol); break;
              case 2: printRobot(grid, row, col, 'v', *maxRow, *maxCol); break;
              case 3: printRobot(grid, row, col, '<', *maxRow, *maxCol); break;
              default: break;
            }
          }
        }
      }

    }

    prevDir = direction;
    printGrid(grid, *maxRow, *maxCol);
  } while (1);

  printf("\n");

  return grid;
}
///////


int main()
{
  	// Robot setup
	pynq_init();
	comms_init();
	edge_sensors_init();
  	stepper_init();
	// connect power pins for sensors and set them high (sensors off)
	gpio_set_direction(POWER_PIN_COLOR_SENSOR, GPIO_DIR_OUTPUT);
	gpio_set_level(POWER_PIN_COLOR_SENSOR, GPIO_LEVEL_HIGH);
	gpio_set_direction(POWER_PIN_DISTANCE_SENSOR_LOWER, GPIO_DIR_OUTPUT);
	gpio_set_level(POWER_PIN_DISTANCE_SENSOR_LOWER, GPIO_LEVEL_HIGH);
	gpio_set_direction(POWER_PIN_DISTANCE_SENSOR_MIDDLE, GPIO_DIR_OUTPUT);
	gpio_set_level(POWER_PIN_DISTANCE_SENSOR_MIDDLE, GPIO_LEVEL_HIGH);
	gpio_set_direction(POWER_PIN_DISTANCE_SENSOR_UPPER, GPIO_DIR_OUTPUT);
	gpio_set_level(POWER_PIN_DISTANCE_SENSOR_UPPER, GPIO_LEVEL_HIGH);
	sleep_msec(200);
  	// connect IIC pins for the sensors that use them
  	switchbox_set_pin(IO_AR_SCL, SWB_IIC0_SCL);
	switchbox_set_pin(IO_AR_SDA, SWB_IIC0_SDA);
	iic_init(IIC0);
	// connect distance sensors
	vl53x distance_sensor_lower;
	vl53x distance_sensor_upper;
	vl53x distance_sensor_middle;
  	if (distance_sensor_init(&distance_sensor_lower, &distance_sensor_upper, &distance_sensor_middle) != 0) {return EXIT_FAILURE;}
	// connect color sensor
  	tcs3472 color_sensor;
  	if (color_sensor_init(&color_sensor) != 0) {printf("Color sensor fucked\n");} // return EXIT_FAILURE;}
  	stepper_enable();
  	stepper_set_speed(30000, 30000);

	/*tcsReading rgb;
	printf("        \n        \n        \n"); // Buffer some space
	for (int i = 0; i < 12; i++)
	{
		i = tcs_get_reading(&color_sensor, &rgb);
		print_colour(rgb.red, rgb.green, rgb.blue); //Used to print colour to screen
		sleep_msec(INTEGRATION_TIME_MS + 20);
	}*/


  //algorithm
  int Rows = 9;
  int Cols = 9;

  // Create initial grid
  GridCell** grid = createGrid(Rows, Cols);

  // Initialize grid elements
  initialiseGrid(grid, Rows, Cols, 0, 0);

  grid = algorithmMovement(grid, &Rows, &Cols, &distance_sensor_lower, &distance_sensor_middle, &distance_sensor_upper, &color_sensor);
  freeGrid(grid, Rows);
  	// Code for execution
  	/*uint32_t iDistance;
  	while (1) // read values 20 times a second for 1 minute
	{
		printf("\r\033[K");
		iDistance = tofReadDistance(&distance_sensor_lower);
		printf("lower distance = %dmm   ", iDistance);
		iDistance = tofReadDistance(&distance_sensor_middle);
		printf("middle distance = %dmm   ", iDistance);
		iDistance = tofReadDistance(&distance_sensor_upper);
		printf("upper distance = %dmm", iDistance);
		fflush(NULL);
	}

  	tcsReading rgb;
	printf("        \n        \n        \n"); // Buffer some space
	for (int i = 0; i < 12; i++)
	{
		i = tcs_get_reading(&color_sensor, &rgb);
		print_colour(rgb.red, rgb.green, rgb.blue); //Used to print colour to screen
		sleep_msec(INTEGRATION_TIME_MS + 20);
	}
	*/
	// while (1) {
	// 	printf("%s    %s\n", (get_edge(LEFT))?"edge":"no edge", (get_edge(RIGHT))?"edge":"no edge");
	// }

  	// Robot destruction code
  	iic_destroy(IIC0);
  	uart_destroy(UART0);
  	pynq_destroy();
  	return EXIT_SUCCESS;
}

// Code to power up color sensor, adapted from tcs3472 library example code
int color_sensor_init(tcs3472* color_sensor)
{
	gpio_set_level(POWER_PIN_COLOR_SENSOR, GPIO_LEVEL_LOW);
	sleep_msec(20);
	uint8_t id;
	int debug;
  	// check connection
	debug = tcs_ping(IIC0, &id);
	if(debug != TCS3472_SUCCES)
	{
		printf("Color sensor connection failure\n");
		return 1;
	}
	printf("Color sensor connection success\n");
	printf("-- ID: %#X\n", id);
	  // connect sensor
	tcs3472 sensor = TCS3472_EMPTY;
	tcs_set_integration(&sensor, tcs3472_integration_from_ms(INTEGRATION_TIME_MS));
	tcs_set_gain(&sensor, x4);
  	debug = tcs_init(IIC0, &sensor);
  	if(debug != TCS3472_SUCCES)
	{
		printf("Color sensor initialization failure\n");
		return 1;
	}
	printf("Color sensor initialization success\n");
	fflush(NULL);
  	// wait one integration cycle to start getting readings
	sleep_msec(INTEGRATION_TIME_MS);
  	*color_sensor = sensor;
	printf("\e[32mAll color sensors connected successfully\e[0m\n");
	fflush(NULL);
  	return 0;
}

int distance_sensor_init(vl53x* distance_sensor_lower, vl53x* distance_sensor_upper, vl53x* distance_sensor_middle)
{
	int debug;
	// set up lower sensor
	gpio_set_level(POWER_PIN_DISTANCE_SENSOR_LOWER, GPIO_LEVEL_LOW);
	sleep_msec(20);
	printf("Initialising Sensor A:\n");
	debug = tofSetAddress(IIC0, 0x29, TOF_ADDRESS_LOW);
	if (debug != 0) {
		printf("Failed to change lower distance sensor address\n");
		return 1;
	}
	printf("Changed lower distance sensor address\n");
	debug = tofPing(IIC0, TOF_ADDRESS_LOW);
	if(debug != 0)
	{
		printf("Failed to ping lower distance sensor\n");
		return 1;
	}
	printf("Successfully pinged lower distance sensor\n");
	// Create a sensor struct
	vl53x sensorA;
	// Initialize the sensor
	debug = tofInit(&sensorA, IIC0, TOF_ADDRESS_LOW, 0);
	if (debug != 0)
	{
		printf("Failed to initialize lower distance sensor\n");
		return 1;
	}
	printf("Successfully initialized lower distance sensor\n");
	// Setup upper sensor
	gpio_set_level(POWER_PIN_DISTANCE_SENSOR_UPPER, GPIO_LEVEL_LOW);
	sleep_msec(20);
	printf("Initialising upper distance sensor\n");
	// Change address for upper sensor too
	debug = tofSetAddress(IIC0, 0x29, TOF_ADDRESS_HIGH);
	if (debug != 0) {
		printf("Failed to change lower distance sensor address\n");
		return 1;
	}
	// Ping upper sensor
	debug = tofPing(IIC0, TOF_ADDRESS_HIGH);
	if(debug != 0)
	{
		printf("Failed to ping upper distance sensor\n");
		return 1;
	}
	printf("Successfully pinged upper distance sensor\n");
	// Create a sensor struct
	vl53x sensorB;
	// Initialize the sensor
	debug = tofInit(&sensorB, IIC0, TOF_ADDRESS_HIGH, 0);
	if (debug != 0)
	{
		printf("Failed to initialize upper distance sensor\n");
		return 1;
	}
	// Setup upper sensor
	printf("Initialising middle distance sensor\n");
	gpio_set_level(POWER_PIN_DISTANCE_SENSOR_MIDDLE, GPIO_LEVEL_LOW);
	sleep_msec(20);
	// Change address for upper sensor too
	debug = tofSetAddress(IIC0, 0x29, TOF_ADDRESS_MIDDLE);
	if (debug != 0) {
		printf("Failed to change middle distance sensor address\n");
		return 1;
	}
	// Ping upper sensor
	debug = tofPing(IIC0, TOF_ADDRESS_MIDDLE);
	if(debug != 0)
	{
		printf("Failed to ping middle distance sensor\n");
		return 1;
	}
	printf("Successfully pinged middle distance sensor\n");
	// Create a sensor struct
	vl53x sensorC;
	// Initialize the sensor
	debug = tofInit(&sensorC, IIC0, TOF_ADDRESS_MIDDLE, 0);
	if (debug != 0)
	{
		printf("Failed to initialize middle distance sensor\n");
		return 1;
	}
	*distance_sensor_lower = sensorA;
	*distance_sensor_upper = sensorB;
	*distance_sensor_middle = sensorC;
	printf("\e[32mAll distance sensors connected successfully\e[0m\n");
	fflush(NULL);
	return 0;
}
