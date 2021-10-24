using namespace std;
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "imdb.h"

const char *const imdb::kActorFileName = "actordata";
const char *const imdb::kMovieFileName = "moviedata";

int *offsets1;
int *offsets2;

imdb::imdb(const string &directory)
{
  const string actorFileName = directory + "/" + kActorFileName;
  const string movieFileName = directory + "/" + kMovieFileName;

  actorFile = acquireFileMap(actorFileName, actorInfo);
  movieFile = acquireFileMap(movieFileName, movieInfo);
}

bool imdb::good() const
{
  return !((actorInfo.fd == -1) ||
           (movieInfo.fd == -1));
}
// you should be implementing these two methods right here...

void imdb::prepare() const{
  char* current1 = (char*)actorFile;
  int size1 = *((int*)current1);  current1 = (char*)current1 + sizeof(int);
  offsets1 = new int [size1];
  for(int q = 0 ; q<size1 ; q++){
    offsets1[q] = *(int*)current1;
    current1 = (char*)current1 + sizeof(int);
  }
  ////////////////////////////////////
  char* current2 = (char*)movieFile;
  int size2 = *((int*)current2);  current2 = (char*)current2 + sizeof(int);
  offsets2 = new int [size2];
  for(int q = 0 ; q<size2 ; q++){
    offsets2[q] = *(int*)current2;
    current2 = current2 + sizeof(int);
  }
}
 int imdb::find(string name , int* offset , const void* searchHere) const
 {
   int size = *((int*)searchHere);
   int low = 0; int high = size-1 ; int mid = (low+high)/2 ;
   while(true){
     if((char*)searchHere + offset[mid] == name) return offset[mid];
     if((char*)searchHere + offset[mid] > name){
       high = mid-1;
       mid = (low+high)/2;
     }
     if((char*)searchHere + offset[mid] < name){
       low = mid+1;
       mid = (low+high)/2;
     }
    if(low>high) break;
   }
  return -1;
}

bool imdb::getCredits(const string &player, vector<film> &films) const
{
  prepare();
  string name = player;

  int far = find(name, offsets1 , actorFile);
  if(far == -1) return false; 
  void* info = (char*)actorFile + far;
  while(*(char*)info !='\0') info = (char*)info + 1;
  bool ind = false;
  if(player.length()%2 == 0){
     info = (char*)info + 2;
     ind = true;
  }else info = (char*)info +1;
  short movieNum = *(short*) info; info = (short*)info + 1;
  if(ind){
    if((player.length())%4 != 0) info = (char*)info + 2;
  }else{
    if((player.length()+3)%4 != 0) info = (char*)info + 2; 
  }
  for(int q = 0 ; q<movieNum ; q++){
    string movieName = "";
    int fir = *(int*)info; info = (int*)info + 1;
    void* movieInformation = (char*)movieFile + fir; 
    while(*(char*)movieInformation != '\0'){
      movieName = movieName + *(char*)movieInformation;
      movieInformation = (char*)movieInformation + 1;
    }
    movieInformation = (char*)movieInformation + 1;
    char year = *((char*)movieInformation); 
    film temp ; temp.title = movieName ; temp.year = 1900 + (int)year;
    films.push_back(temp);
  }
  return true;
}
bool imdb::getCast(const film &movie, vector<string> &players) const
{
  int far = find(movie.title, offsets2 , movieFile);
  if(far == -1 ) return false;
  void* info = (char*)movieFile + far;
  while(*(char*)info !='\0') info = (char*)info + 1;
  bool ind = false;
  info = (char*)info +1;
  char* year = (char*)info; info = (char*)info+1;
  if(movie.title.length()%2==1){
   info = (char*)info+1;
   ind = true;
  }
  short actorsNum = *(short*)info; info = (short*)info + 1;
  if(ind){
    if((movie.title.length()+1)%4 != 0) info = (char*)info + 2;
  }else{
    if(movie.title.length()%4 != 0) info = (char*)info + 2;
  }
  for(int q = 0 ; q<actorsNum ; q++){
    string actorsName = "";
    int fir = *(int*)info; info = (int*)info +1;
    void* actorInformation = (char*)actorFile + fir;
    while(*(char*)actorInformation !='\0'){
      actorsName = actorsName + *(char*)actorInformation;
      actorInformation = (char*)actorInformation + 1;
    } 
    players.push_back(actorsName);
  }
  return true;
}
imdb::~imdb()
{
  releaseFileMap(actorInfo);
  releaseFileMap(movieInfo);
}

// ignore everything below... it's all UNIXy stuff in place to make a file look like
// an array of bytes in RAM..
const void *imdb::acquireFileMap(const string &fileName, struct fileInfo &info)
{
  struct stat stats;
  stat(fileName.c_str(), &stats);
  info.fileSize = stats.st_size;
  info.fd = open(fileName.c_str(), O_RDONLY);
  return info.fileMap = mmap(0, info.fileSize, PROT_READ, MAP_SHARED, info.fd, 0);
}

void imdb::releaseFileMap(struct fileInfo &info)
{
  if (info.fileMap != NULL)
    munmap((char *)info.fileMap, info.fileSize);
  if (info.fd != -1)
    close(info.fd);
}
