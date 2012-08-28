uint32_t osmstylenum (char* dbrv) {
int rv = 0;
if (!(strcmp(dbrv, 
// alphabetical order of 30 most common street types
"residential"
          ))) { // swap abandoned with #16 residential, else alphabetical
    return rv;
  }
  rv++;
  if (!(strcmp(dbrv, 
"bridleway"
          ))) {
    return rv;
  }
  rv++;
  if (!(strcmp(dbrv, 
"construction"
          ))) {
    return rv;
  }
  rv++;
  if (!(strcmp(dbrv, 
"crossing"
          ))) {
    return rv;
  }
  rv++;
  if (!(strcmp(dbrv, 
"cycleway"
          ))) {
    return rv;
  }
  rv++;
  if (!(strcmp(dbrv, 
"footway"
          ))) {
    return rv;
  }
  rv++;
  if (!(strcmp(dbrv, 
"footway_unconstructed"
          ))) {
    return rv;
  }
  rv++;
  if (!(strcmp(dbrv, 
"living_street"
          ))) {
    return rv;
  }
  rv++;
  if (!(strcmp(dbrv, 
"motorway"
          ))) {
    return rv;
  }
  rv++;
  if (!(strcmp(dbrv, 
"motorway_link"
          ))) {
    return rv;
  }
  rv++;
  if (!(strcmp(dbrv, 
"path"
          ))) {
    return rv;
  }
  rv++;
  if (!(strcmp(dbrv, 
"pedestrian"
          ))) {
    return rv;
  }
  rv++;
  if (!(strcmp(dbrv, 
"platform"
          ))) {
    return rv;
  }
  rv++;
  if (!(strcmp(dbrv, 
"primary"
          ))) {
    return rv;
  }
  rv++;
  if (!(strcmp(dbrv, 
"primary_link"
          ))) {
    return rv;
  }
  rv++;
  if (!(strcmp(dbrv, 
"proposed"
          ))) {
    return rv;
  }
  rv++;
  if (!(strcmp(dbrv, 
"raceway"
          ))) {
    return rv;
  }
  rv++;
  if (!(strcmp(dbrv, 
"abandoned"
          ))) {
    return rv;
  }
  rv++;
  if (!(strcmp(dbrv, 
"road"
          ))) {
    return rv;
  }
  rv++;
  if (!(strcmp(dbrv, 
"secondary"
          ))) {
    return rv;
  }
  rv++;
  if (!(strcmp(dbrv, 
"secondary_link"
          ))) {
    return rv;
  }
  rv++;
  if (!(strcmp(dbrv, 
"service"
          ))) {
    return rv;
  }
  rv++;
  if (!(strcmp(dbrv, 
"service; residential"
          ))) {
    return rv;
  }
  rv++;
  if (!(strcmp(dbrv, 
"steps"
          ))) {
    return rv;
  }
  rv++;
  if (!(strcmp(dbrv, 
"tertiary"
          ))) {
    return rv;
  }
  rv++;
  if (!(strcmp(dbrv, 
"tertiary_link"
          ))) {
    return rv;
  }
  rv++;
  if (!(strcmp(dbrv, 
"track"
          ))) {
    return rv;
  }
  rv++;
  if (!(strcmp(dbrv, 
"trunk"
          ))) {
    return rv;
  }
  rv++;
  if (!(strcmp(dbrv, 
"trunk_link"
          ))) {
    return rv;
  }
  rv++;
  if (!(strcmp(dbrv, 
"unclassified"
          ))) {
    return rv;
  }
  return rv; // non-explicit unclassified
}
