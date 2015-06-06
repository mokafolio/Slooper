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
   while(true)
   {
      std::cout<<"CURRENT TIME: "<<player.currentTime()<<std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
   }
   return EXIT_SUCCESS;
}


