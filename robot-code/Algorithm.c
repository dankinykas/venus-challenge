#include <libpynq.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>


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
} GridCell;

typedef struct {
  int curDir;
  int newDir;
  int steps;
} RobotData;


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
      } else {
        switch (grid[i-1][j-1].colour){ // print grid
        case RED:         printf(" \033[31m%c\033[0m ", grid[i-1][j-1].status);        break;
        case BLUE:        printf(" \033[34m%c\033[0m ", grid[i-1][j-1].status);        break;
        case GREEN:       printf(" \033[32m%c\033[0m ", grid[i-1][j-1].status);        break;
        case WHITE:       printf(" \033[37m%c\033[0m ", grid[i-1][j-1].status);        break;
        case WHITEBACK:   printf("\033[47m%3c\033[0m", grid[i-1][j-1].status);         break;
        case BLACK:       printf(" \033[30;47m%c\033[0m ",grid[i-1][j-1].status);      break;
        case YELLOWBACK:  printf(" \033[1;30;43m%c\033[0m ",grid[i-1][j-1].status);    break;
        case YELLOW:      printf(" \033[1;33m%c\033[0m ", grid[i-1][j-1].status);        break;
        case GRAY:        printf(" \033[30m%c\033[0m ", grid[i-1][j-1].status);        break;
        case BROWN:       printf(" \033[38;5;94m%c\033[0m ", grid[i-1][j-1].status); break;
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
      } else {
        grid[i][j].status = '#';
        grid[i][j].colour = GRAY;
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
              if(grid[i][h+1].status == '^' || grid[i][h+1].status == '>' || grid[i][h+1].status == 'v' || grid[i][h+1].status == '<'){
                grid[i][h].status = grid[i][h+1].status;
                grid[i][h].colour = YELLOW;
              } else {
                grid[i][h].status = '#';
                grid[i][h].colour = GRAY;
              }
            } else {
              grid[i][h].status = grid[i][h-1].status;
              grid[i][h].colour = grid[i][h-1].colour;
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
              if(grid[h+1][j].status == '^' || grid[h+1][j].status == '>' || grid[h+1][j].status == 'v' || grid[h+1][j].status == '<'){
                grid[h][j].status = grid[h+1][j].status;
                grid[h][j].colour = YELLOW;
              } else {
                grid[h][j].status = '#';
                grid[h][j].colour = GRAY;
              }
            } else {
              grid[h][j].status = grid[h-1][j].status;
              grid[h][j].colour = grid[h-1][j].colour;
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
    free(grid[i]); // Free memory for each row's columns
  }

  free(grid); // Free memory for rows
}


// Fucntion to read keys
// Function to set the terminal to raw mode
void set_raw_mode(struct termios *original) {
    struct termios raw;

    // Get current terminal attributes
    tcgetattr(STDIN_FILENO, original);

    // Make a copy of the current attributes to modify
    raw = *original;

    // Input modes: no break, no CR to NL, no parity check, no strip char, no start/stop output control
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);

    // Output modes: disable post processing
    raw.c_oflag &= ~(OPOST);

    // Control modes: set 8 bit chars
    raw.c_cflag |= (CS8);

    // Local modes: echoing off, canonical mode off, no extended functions, no signal chars
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

    // Control characters: set minimum number of bytes and timer
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    // Set the terminal attributes
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

// Function to restore the terminal to its original settings
void restore_terminal_mode(struct termios *original) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, original);
}


/*void* stepperMotor(void* tmpRobot){

  RobotData* robot = (RobotData*) tmpRobot;
  int turn = 0;


  if(robot->curDir != robot->newDir){
    for (int i = robot->curDir; i < 5; i++){
      turn++;
      if(i > 3){
        i = 0;
      }
      if(i == robot->newDir){
        break;
      }
    }
    if(turn == 3){
      turn = -1;
    }
    stepper_steps(TURN*turn, TURN*turn);
  }

  stepper_steps(-robot->steps, robot->steps);

  return NULL;
} */


// Function to move inside the grid
GridCell** movement(GridCell** grid, int* maxRow, int* maxCol){

  printGrid(grid, *maxRow, *maxCol);

  int button = -1;
  int row = 0;
  int col = 0;
  int minRow = grid[0][0].row;
  int minCol = grid[0][0].col;
  int rowMax = grid[*maxRow-1][*maxCol-1].row;
  int colMax = grid[*maxRow-1][*maxCol-1].col;
  int loop = 1;
  int prevDir = 0;
  char prevStatus;
  int prevColour = 0;

  /*RobotData robot;
  robot.curDir = 0; // 0 == north, 1 == east, 2 == south, 3 == west (north == start direction)
  robot.newDir = 0;
  robot.steps = 10; // number of steps per movement
  pthread_t thread2;*/

  struct termios original;
  char c;


  //search starting/continuation point
  for (int i = 0; i < *maxRow; i++){
    for (int j = 0; j < *maxCol; j++){
      if(grid[i][j].status == '^' || grid[i][j].status == '>' || grid[i][j].status == 'v' || grid[i][j].status == '<'){
        row = grid[i][j].row;
        col = grid[i][j].col;
        break;
      }
      if(grid[i][j].status == 'S' && row == 0 && col == 0){
        row = grid[i][j].row;
        col = grid[i][j].col;
      }
    }
  }

  do{
    //check movement input
    if(get_button_state(0)){ button = 3; } // west
    else if(get_button_state(1)){ button = 2; } // south
    else if(get_button_state(2)){ button = 1; } // east
    else if(get_button_state(3)){ button = 0; } // north

    set_raw_mode(&original);

    read(STDIN_FILENO, &c, 1);
    if(c == 'w'){ button = 0; printf("\n");} // north
    else if(c == 'd'){ button = 1; printf("\n");} // east
    else if(c == 's'){ button = 2; printf("\n");} // south
    else if(c == 'a'){ button = 3; printf("\n"); } // west
    else if(c == 'q'){ restore_terminal_mode(&original); break; } // stop

    restore_terminal_mode(&original);


    if(button != -1){
      //robot.newDir = button;
      if(prevDir != button){
        for (int i = 0; i < *maxRow; i++){
          for (int j = 0; j < *maxCol; j++){
            if(grid[i][j].status == '^' || grid[i][j].status == '>' || grid[i][j].status == 'v' || grid[i][j].status == '<'){
              switch(button){
                case 0: grid[i][j].status = '^'; break;
                case 1: grid[i][j].status = '>'; break;
                case 2: grid[i][j].status = 'v'; break;
                case 3: grid[i][j].status = '<'; break;
                default: break;
              }
            }
          }
        }
      } else {

        for (int i = 0; i < *maxRow; i++){
          for (int j = 0; j < *maxCol; j++){
            if(grid[i][j].status == '^' || grid[i][j].status == '>' || grid[i][j].status == 'v' || grid[i][j].status == '<'){
              
              grid[i][j].status = prevStatus;
              grid[i][j].colour = prevColour;
              
            }
          }
        }

        //pthread_create(&thread2, NULL, stepperMotor, &robot);

        switch (button){
        case 0: /// up

        row--;
        if(row < minRow){
          grid = expandGrid(grid, *maxRow+1, *maxCol, *maxRow, *maxCol);
          (*maxRow)++;
          re_initialiseGrid(grid, *maxRow, *maxCol, -1, 0);
          minRow = row;
        }
        printf("\nup\n");
        break;
        case 1: /// right

        col++;
        if(col > colMax){
          grid = expandGrid(grid, *maxRow, *maxCol+1, *maxRow, *maxCol);
          (*maxCol)++;
          re_initialiseGrid(grid, *maxRow, *maxCol, 0, 1);
          colMax++;
        }
        printf("\nright\n");
        break;
        case 2: /// down

        row++;
        if(row > rowMax){
          grid = expandGrid(grid, *maxRow+1, *maxCol, *maxRow, *maxCol);
          (*maxRow)++;
          re_initialiseGrid(grid, *maxRow, *maxCol, 1, 0);
          rowMax++;
        }
        printf("\ndown\n");
        break;
        case 3: /// left

        col--;
        if(col < minCol){
          grid = expandGrid(grid, *maxRow, *maxCol+1, *maxRow, *maxCol);
          (*maxCol)++;
          re_initialiseGrid(grid, *maxRow, *maxCol, 0, -1);
          minCol = col;
        }
        printf("\nleft\n");
        break;
        default:
        break;
        }

        //pthread_join(thread2, NULL);

        for (int i = 0; i < *maxRow; i++){
          for (int j = 0; j < *maxCol; j++){
            if(grid[i][j].row == row && grid[i][j].col == col && grid[i][j].status != 'S'){
              prevStatus = grid[i][j].status;
              prevColour = grid[i][j].colour;

              switch(button){
                case 0: grid[i][j].status = '^'; break;
                case 1: grid[i][j].status = '>'; break;
                case 2: grid[i][j].status = 'v'; break;
                case 3: grid[i][j].status = '<'; break;
                default: break;
              }
              grid[i][j].colour = YELLOW;
            }
          }
        }
      
      }
      //robot.curDir = button;
      prevDir = button;
      button = -1;
      printGrid(grid, *maxRow, *maxCol);
      //sleep_msec(150);
      printf("For exit enter 'q'\n");

    }

  } while (loop);
  printf("\n");

  return grid;
}


// Function for selecting obstacles for map
char selectMap(){

  char fill;
  int loop = 0;

  //unexplored = #
  //cliff or border = =
  //3*3 block = 3
  //6*6 block = 6
  //mountain = M

  do{
    loop = 0;

    printf("\nSelect object:\n-Unexplored = '#'\n-Border = '='\n-3*3 Block = '3'\n-6*6 Block = '6'\n-Cliff: 'C'\n-Mountain = 'M'\n:");
    scanf(" %c", &fill);

    switch (fill){
    case '#':
    return '#';
    break;
    case '=':
    return ' ';
    break;
    case 'C':
    return 'C';
    break;
    case '3':
    return '3';
    break;
    case '6':
    return '6';
    break;
    case 'M':
    return 'M';
    break;
    default:
    printf("Unknown command\n");
    loop = 1;
    break;
    }
  } while(loop);

  return 'c';
}


// Function for drawing a line
void drawLine(GridCell** grid, int x1, int y1, int x2, int y2){ 

  int dx = abs(x2 - x1);
  int dy = abs(y2 - y1);
  int dirx = 0;
  int diry = 0;
  int slope_error = dx - dy;
  int loop = 0;

  int x = x1;
  int y = y1;

  if(x1 < x2){
    dirx = 1;
  } else {
    dirx = -1;
  }
  if(y1 < y2){
    diry = 1;
  } else {
    diry = -1;
  }

  while (1) {

    if(loop == 1){
      slope_error = dx - dy;
      x = x1;
      y = y1;
      loop++;
    }

    if(loop > 0){
      grid[y][x].status = ' ';
      grid[y][x].colour = WHITEBACK;
    } else if(grid[y][x].status != '#' && grid[y][x].status != ' ' && grid[y][x].status != 'R'){
      printf("Error: Line obstructed\n");
      grid[y1][x1].status = '#';
      grid[y1][x1].colour = GRAY;
      break;
    }

    if (x == x2 && y == y2){
      if(loop == 0){
        loop++;
      } else {
        break;
      }
    }

    int e2 = 2 * slope_error;

    if (e2 > -dy) {
      slope_error -= dy;
      x += dirx;
    }

    if (e2 < dx) {
      slope_error += dx;
      y += diry;
    }
  }

} 


// Function to place objects in the map
GridCell** fillMap(GridCell** grid, int row1, int col1, int row2, int col2, char select){

  int loop = 0;
  int tmp = 0;
  char cmd;


  if(select == '3' || select == '6'){
    
    do{
      loop = 0;

      printf("What colour?\n-Red = r\n-Blue = b\n-Green = g\n-White = w\n-Black = B\n");
      scanf(" %c", &cmd);
      switch (cmd){
      case 'r':
      if(select == '3'){
        for (int i = row1; i < row1+BLOCKSIZE; i++){
          for (int j = col1; j < col1+BLOCKSIZE; j++){
            grid[i][j].status = '3';
            grid[i][j].colour = RED;
          }
        }
      } else if(select == '6'){
        for (int i = row1; i < row1+BLOCKSIZE*2; i++){
          for (int j = col1; j < col1+BLOCKSIZE*2; j++){
            grid[i][j].status = '6';
            grid[i][j].colour = RED;
          }
        }
      }
      break;
      case 'b':
      if(select == '3'){
        for (int i = row1; i < row1+BLOCKSIZE; i++){
          for (int j = col1; j < col1+BLOCKSIZE; j++){
            grid[i][j].status = '3';
            grid[i][j].colour = BLUE;
          }
        }
      } else if(select == '6'){
        for (int i = row1; i < row1+BLOCKSIZE*2; i++){
          for (int j = col1; j < col1+BLOCKSIZE*2; j++){
            grid[i][j].status = '6';
            grid[i][j].colour = BLUE;
          }
        }
      }
      break;
      case 'g':
      if(select == '3'){
        for (int i = row1; i < row1+BLOCKSIZE; i++){
          for (int j = col1; j < col1+BLOCKSIZE; j++){
            grid[i][j].status = '3';
            grid[i][j].colour = GREEN;
          }
        }
      } else if(select == '6'){
        for (int i = row1; i < row1+BLOCKSIZE*2; i++){
          for (int j = col1; j < col1+BLOCKSIZE*2; j++){
            grid[i][j].status = '6';
            grid[i][j].colour = GREEN;
          }
        }
      }
      break;
      case 'w':
      if(select == '3'){
        for (int i = row1; i < row1+BLOCKSIZE; i++){
          for (int j = col1; j < col1+BLOCKSIZE; j++){
            grid[i][j].status = '3';
            grid[i][j].colour = WHITE;
          }
        }
      } else if(select == '6'){
        for (int i = row1; i < row1+BLOCKSIZE*2; i++){
          for (int j = col1; j < col1+BLOCKSIZE*2; j++){
            grid[i][j].status = '6';
            grid[i][j].colour = WHITE;
          }
        }
      }
      break;
      case 'B':
      if(select == '3'){
        for (int i = row1; i < row1+BLOCKSIZE; i++){
          for (int j = col1; j < col1+BLOCKSIZE; j++){
            grid[i][j].status = '3';
            grid[i][j].colour = BLACK;
          }
        }
      } else if(select == '6'){
        for (int i = row1; i < row1+BLOCKSIZE*2; i++){
          for (int j = col1; j < col1+BLOCKSIZE*2; j++){
            grid[i][j].status = '6';
            grid[i][j].colour = BLACK;
          }
        }
      }
      break;
      default:
      printf("Unknown command?\n");
      loop = 1;
      break;
      }

    } while (loop);
  } else if(select == ' '){

    drawLine(grid, col1, row1, col2, row2);

  } else {

    if(row1 > row2){
      tmp = row1;
      row1 = row2;
      row2 = tmp;
    }
    if(col1 > col2){
      tmp = col1;
      col1 = col2;
      col2 = tmp;
    }

    if(select == 'M' || select == 'C'){

      for (int i = row1; i <= row2; i++){
        for (int j = col1; j <= col2; j++){
          if(loop == 0){
            if(grid[i][j].status != '#' && grid[i][j].status != select && grid[i][j].status != 'R'){
              printf("Error: Area occupied\n");
              loop = 1;
              i = row1-1;
              j = col1;
              break;
            }
          } else if(loop == 1){
            if(grid[i][j].colour == YELLOWBACK){
              grid[i][j].status = '#';
              grid[i][j].colour = GRAY;
              return grid;
            }
          }
        }
      }

      for (int i = row1; i <= row2; i++){
        for (int j = col1; j <= col2; j++){
          grid[i][j].status = select;
          grid[i][j].colour = BROWN;
        }
      }

    } else if(select == '#'){

      for (int i = row1; i <= row2; i++){
        for (int j = col1; j <= col2; j++){
          if(loop == 0){
            if(grid[i][j].status == 'S'){
              printf("Error: Not allowed to delete S(tart)\n");
              loop--;
              i = row1-1;
              j = col1;
              break;
            } else if(i == row2 && j == col2){
              loop++;
              i = row1-1;
              j = col1;
            }
          } else if(loop < 0){
            if(grid[i][j].colour == YELLOWBACK){
              grid[i][j].colour = GRAY;
              break;
            } 
          } else if(loop > 0){
            grid[i][j].status = '#';
            grid[i][j].colour = GRAY;
          }
        }
      }

    }
    
  }

  return grid;
}


// Function to create a map
GridCell** createMap(GridCell** grid, int* maxRow, int* maxCol){

  printGrid(grid, *maxRow, *maxCol);

  int button = -1;
  int row = 0;
  int col = 0;
  char prevStatus;
  int prevColour = 0;
  int minRow = grid[0][0].row;
  int minCol = grid[0][0].col;
  int rowMax = grid[*maxRow-1][*maxCol-1].row;
  int colMax = grid[*maxRow-1][*maxCol-1].col;
  int loop = 1;

  struct termios original;
  char c;
  int onoff = 0;
  char select;
  int spot1row = 0;
  int spot1col = 0;
  int spot2row = 0;
  int spot2col = 0;
  int state = 0; // in what process of editing, 0 == nothing done yet, 1 == first position chosen, 2 == second position chosen
  int curi = 1; // grid coordinates
  int curj = 1;


  //unexplored = #
  //start = S
  //current postition = R(obot)
  //explored empty = *
  //explored treasure = !

  //search starting/continuation point
  for (int i = 0; i < *maxRow; i++){
    for (int j = 0; j < *maxCol; j++){
      if(grid[i][j].status == 'R'){
        row = grid[i][j].row;
        col = grid[i][j].col;
        break;
      }
      if(grid[i][j].status == 'S' && row == 0 && col == 0){
        row = grid[i][j].row;
        col = grid[i][j].col;
      }
    }
  }

  do{
    //check movement input
    if(get_button_state(0)){ button = 3; } // west
    else if(get_button_state(1)){ button = 2; } // south
    else if(get_button_state(2)){ button = 1; } // east
    else if(get_button_state(3)){ button = 0; } // north

    set_raw_mode(&original);

    read(STDIN_FILENO, &c, 1);
    if(c == 'w'){ button = 0; printf("\n");} // north
    else if(c == 'd'){ button = 1; printf("\n");} // east
    else if(c == 's'){ button = 2; printf("\n");} // south
    else if(c == 'a'){ button = 3; printf("\n"); } // west
    else if(c == 'q' && onoff == 0){ // stop
      restore_terminal_mode(&original);
      grid[curi][curj].status = prevStatus;
      grid[curi][curj].colour = prevColour;
      break; 
    }
    else if(c == 'c'){ // create map
      restore_terminal_mode(&original);
      if(onoff == 0){
        printf("\nEdit mode ON\n");
        onoff = 1;
        select = selectMap();
        printGrid(grid, *maxRow, *maxCol);
        printf("For placing marker press 'space'\n");
      } else if(onoff == 1){
        if(spot1row == spot1col && spot1row == 0){
          printf("\nEdit mode OFF\n");
          onoff = 0;
        } else {
          printf("Finish current edit first\n");
        }
      }
    } else if(c == ' ' && onoff == 1){
      restore_terminal_mode(&original);
      if(state == 0){
        state++;
        button = 0;


        if(select == '3' || select == '6'){ // for 3 or 6 selected
          if(select == '3'){ //checks if 3 will be put out of bounds or in an already occupied space
            if((curi + BLOCKSIZE) > *maxRow){
              printf("Error: Object out of bounds\n");
              state = -1;
            } else if((curj + BLOCKSIZE) > *maxCol){
              printf("Error: Object out of bounds\n");
              state = -1;
            } else {
              for (int i = curi; i < curi+BLOCKSIZE; i++){
                for (int j = curj; j < curj+BLOCKSIZE; j++){
                  if(i == curi && j == curj){
                    if(prevStatus != '#'){
                      printf("Error: Space occupied\n");
                      state = -1;
                    }
                  } else if(grid[i][j].status != '#'){
                    printf("Error: Space occupied\n");
                    state = -1;
                  }
                }
              }
            }
          } else if(select == '6'){ //checks if 6 will be put out of bounds or in an already occupied space
            if((curi + BLOCKSIZE*2) > *maxRow){
              printf("Error: Object out of bounds\n");
              state = -1;
            } else if((curj + BLOCKSIZE*2) > *maxCol){
              state = -1;
            } else {
              for (int i = curi; i < curi+BLOCKSIZE*2; i++){
                for (int j = curj; j < curj+BLOCKSIZE*2; j++){
                  if(i == curi && j == curj){
                    if(prevStatus != '#'){
                      printf("Error: Space occupied\n");
                      state = -1;
                    }
                  } else if(grid[i][j].status != '#'){
                    printf("Error: Space occupied\n");
                    state = -1;
                  }
                }
              }
            }
          }

          state++;
        } else if(select != '#' && prevStatus != '#'){
          printf("Error: Space occupied\n");
          state = 0;
        } else if(select == '#' && prevStatus == 'S'){
          printf("Error: Not allowed to delete S(tart)\n");
          state = 0;
        }

        if(state != 0){
          spot1row = curi;
          spot1col = curj;
          grid[curi][curj].status = select;
          grid[curi][curj].colour = YELLOWBACK;
        }

      } else if(state == 1){
        state++;
        button = 0;

        if(select != '#' && prevStatus != '#'){
          if(grid[curi][curj].row != grid[spot1row][spot1col].row || grid[curi][curj].col != grid[spot1row][spot1col].col){
            printf("Error: Space occupied1\n");
            state = 0;
            grid[spot1row][spot1col].status = '#';
            grid[spot1row][spot1col].colour = GRAY;
            spot1row = 0;
            spot1col = 0;
          }
        } else if(select == '#' && prevStatus == 'S'){
          printf("Error: Not allowed to delete S(tart)\n");
          state = 0;
          grid[spot1row][spot1col].colour = GRAY;
          spot1row = 0;
          spot1col = 0;
        }

        if(state != 0){
          spot2row = curi;
          spot2col = curj;
        }
      }
    }

    restore_terminal_mode(&original);

    if(state == 2){
      grid = fillMap(grid, spot1row, spot1col, spot2row, spot2col, select);
      state = 0;
      onoff = 0;
      spot1row = 0;
      spot1col = 0;
      spot2row = 0;
      spot2col = 0;
    }


    if(button != -1){

      for (int i = 0; i < *maxRow; i++){
        for (int j = 0; j < *maxCol; j++){
          if(grid[i][j].status == 'R'){
            grid[i][j].status = prevStatus;
            grid[i][j].colour = prevColour;
          }
        }
      }


      switch (button){
      case 0: /// up

      row--;
      if(row < minRow){
        grid = expandGrid(grid, *maxRow+1, *maxCol, *maxRow, *maxCol);
        (*maxRow)++;
        re_initialiseGrid(grid, *maxRow, *maxCol, -1, 0);
        minRow = row;
        spot1row++;
      }
      printf("\nup\n");
      break;
      case 1: /// right

      col++;
      if(col > colMax){
        grid = expandGrid(grid, *maxRow, *maxCol+1, *maxRow, *maxCol);
        (*maxCol)++;
        re_initialiseGrid(grid, *maxRow, *maxCol, 0, 1);
        colMax++;
      }
      printf("\nright\n");
      break;
      case 2: /// down

      row++;
      if(row > rowMax){
        grid = expandGrid(grid, *maxRow+1, *maxCol, *maxRow, *maxCol);
        (*maxRow)++;
        re_initialiseGrid(grid, *maxRow, *maxCol, 1, 0);
        rowMax++;
      }
      printf("\ndown\n");
      break;
      case 3: /// left

      col--;
      if(col < minCol){
        grid = expandGrid(grid, *maxRow, *maxCol+1, *maxRow, *maxCol);
        (*maxCol)++;
        re_initialiseGrid(grid, *maxRow, *maxCol, 0, -1);
        minCol = col;
        spot1col++;
      }
      printf("\nleft\n");
      break;
      default:
      break;
      }

      for (int i = 0; i < *maxRow; i++){
        for (int j = 0; j < *maxCol; j++){
          if(grid[i][j].row == row && grid[i][j].col == col && grid[i][j].status != 'S'){
            prevStatus = grid[i][j].status;
            prevColour = grid[i][j].colour;
            grid[i][j].status = 'R';
            grid[i][j].colour = YELLOW;
            curi = i;
            curj = j;
          }
        }
      }
      
      button = -1;
      printGrid(grid, *maxRow, *maxCol);
      if(onoff == 0){
        printf("For exit enter 'q'\nFor edit enter 'c'\n");
      } else if(onoff == 1){
        printf("For placing marker press 'space'\n");
      }

    }

  } while (loop);

  printf("\n");

  return grid;
}


// Function to copy map to grid
GridCell** copyMap(GridCell** grid, int* row, int* col, GridCell** map, int mapRow, int mapCol){

  freeGrid(grid, *row);
  *row = 1;
  *col = 1;
  grid = createGrid(*row, *col);
  initialiseGrid(grid, *row, *col, 0, 0);
  
  grid = expandGrid(grid, mapRow, mapCol, *row, *col);

  for (int i = 0; i < mapRow; i++){
    for (int j = 0; j < mapCol; j++){
      grid[i][j].row = map[i][j].row;
      grid[i][j].col = map[i][j].col;
      grid[i][j].status = map[i][j].status;
      grid[i][j].colour = map[i][j].colour;
    }
  }

  *row = mapRow;
  *col = mapCol;

  return grid;
  
}



int main() {
  pynq_init();
  display_t display;
  display_init(&display);
  leds_init_onoff();
  buttons_init();
  //stepper_init();

  //stepper_enable();

  int Rows = 3;
  int Cols = 3;
  int mapRows = 3;
  int mapCols = 3;
  int newRows = 0;
  int newCols = 0;
  char cmd;
  char gm;

  // Create initial grid
  GridCell** grid = createGrid(Rows, Cols);

  // Initialize grid elements
  initialiseGrid(grid, Rows, Cols, 0, 0);


  // Create initial map
  GridCell** map = createGrid(mapRows, mapCols);
  initialiseGrid(map, mapRows, mapCols, 0, 0);


  // Print initial grid
  printf("Initial Grid:\n");
  printGrid(grid, Rows, Cols);


  do{

    printf("-Expand: e\n-Print: p\n-Reset: r\n-Move: m\n-Create map: c\n-Duplicate map to grid: d\n-Quit: q\n Command? ");
    scanf(" %c", &cmd);
    printf("\n");

    switch (cmd){
    case 'e': // Expand the grid
    do{
      printf("Expand grid or map?\n-Grid: g\n-Map: m\nCommand: ");
      scanf(" %c", &gm);
    } while (gm != 'g' && gm != 'm');
    printf("\n");

    printf("Expand rows by: ");
    scanf(" %d", &newRows);
    printf("Expand columns by: ");
    scanf(" %d", &newCols);

    // Reallocate memory for expanded grid
    if(gm == 'g'){
      newRows = newRows + Rows;
      newCols = newCols + Cols;
      grid = expandGrid(grid, newRows, newCols, Rows, Cols);
      initialiseGrid(grid, newRows, newCols, Rows, Cols);
      Rows = newRows;
      Cols = newCols;
    } else if(gm == 'm'){
      newRows = newRows + mapRows;
      newCols = newCols + mapCols;
      map = expandGrid(map, newRows, newCols, mapRows, mapCols);
      initialiseGrid(map, newRows, newCols, mapRows, mapCols);
      mapRows = newRows;
      mapCols = newCols;
    }
    break;
    case 'p':
    printf("Print grid or map?\n-Grid: g\n-Map: m\nCommand: ");
    scanf(" %c", &gm);
    printf("\n");

    if(gm == 'g'){
      printf("Grid:\n");
      printGrid(grid, Rows, Cols);
    } else if(gm == 'm'){
      printf("Map:\n");
      printGrid(map, mapRows, mapCols);
    }
    break;
    case 'm':
    grid = movement(grid, &Rows, &Cols);
    break;
    case 'c': // map maker
    map = createMap(map, &mapRows, &mapCols);
    break;
    case 'r':
    printf("Reset grid or map?\n-Grid: g\n-Map: m\nCommand: ");
    scanf(" %c", &gm);
    printf("\n");

    if(gm == 'g'){
      freeGrid(grid, Rows);
      Rows = 3;
      Cols = 3;
      grid = createGrid(Rows, Cols);
      initialiseGrid(grid, Rows, Cols, 0, 0);
    } else if(gm == 'm'){
      freeGrid(map, mapRows);
      mapRows = 3;
      mapCols = 3;
      map = createGrid(mapRows, mapCols);
      initialiseGrid(map, mapRows, mapCols, 0, 0);
    }
    break;
    case 'd':
    grid = copyMap(grid, &Rows, &Cols, map, mapRows, mapCols);
    break;
    case 'q':
    printf("Bye!\n");
    // Free memory allocated for the grid
    freeGrid(grid, Rows);
    freeGrid(map, mapRows);
    break;
    default:
    printf("Unknown\n");
    break;
    }
  } while (cmd != 'q');


  //stepper_destroy();
  buttons_destroy();
  leds_destroy();
  display_destroy(&display);
  pynq_destroy();
  return 0;
}
