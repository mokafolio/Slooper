#include <Slooper/VideoPlayer/VideoPlayer.hpp>

int main (int argc, char **argv)
{
   if (argc < 2) {
      printf("Usage: %s <filename>\n", argv[0]);
      exit(1);
   }
   bcm_host_init();
   //return video_decode_test(argv[1]);
   slooper::videoPlayer::VideoPlayer player(argv[1]);
   if(player.error())
   {
      std::cout<<"Could not initialize video player: "<<player.errorMessage()<<std::endl;
      return EXIT_FAILURE;
   }
   //std::chrono::system_clock::duration timeSinceLastToggle(0);
   auto startTime = std::chrono::system_clock::now();
   bool bIsPaused = false;
   while(true)
   {
      if(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - startTime).count() >= 1000)
      {
         //std::cout<<"TOGGLE"<<std::endl;
         if(!bIsPaused)
         {
            player.pause();
            /*if(player.error())
            {
               std::cout<<player.errorMessage()<<std::endl;
            }*/
            //std::cout<<"PAUSE VIDEO"<<std::endl;
         }
         else
         {
            //std::cout<<"UNPAUSE VIDEO"<<std::endl;
            player.resume();
            /*if(player.error())
            {
               std::cout<<player.errorMessage()<<std::endl;
            }*/
         }
         startTime = std::chrono::system_clock::now();
         bIsPaused = !bIsPaused;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
   }
   return EXIT_SUCCESS;
}


