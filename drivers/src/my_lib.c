


int cube(int x){
    return x*x*x;
}

int is_alpha(char x){
    int y= x;    
    if((y>=65 && y<=90)||(y>=97 && y<=122)){
        return 1;
    }
    else {
        return 0;
    }
}

int is_digit(char x){
    int y= x;
    if(y>=48 && y<=57){
        return 1;
    }
    else {
        return 0;
    }
}