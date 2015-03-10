
#include <memory>
#include "data.hpp"
#include "rewrite.hpp"
#include "cog.hpp"

using namespace std;

void sortArray(CogHandle h)
{
  CogPtr cog = h->get(); // Grab the functional version of this object.
  
  if(cog->type == COG_ARRAY){
    CogPtr repl = ((ArrayCog *)cog.get())->sortedCog();
    h->put(repl);
  }
}
