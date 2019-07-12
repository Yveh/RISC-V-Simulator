#ifndef RISC_V_PREDICTOR
#define RISC_V_PREDICTOR

template<unsigned int _len = 12>
class predictor_t {
	private:
		int all, corr;
		char *arr;
		
	public:
		predictor_t() {
			all = corr = 0;
			arr = new char[1 << _len];
			memset(arr, 0 ,sizeof(arr));
		}
		bool get(unsigned int x) {
			x = x & ((1 << _len) - 1);
			return arr[x] >= 2;
		}
		void upd(unsigned int x, bool val) {
			x = x & ((1 << _len) - 1);
			if (val)
				arr[x] = std::min(arr[x] + 1, 3);
			else
				arr[x] = std::max(arr[x] - 1, 0);
		}
		void calc(bool val) {
			++all;
			corr += !val;
		}
		~predictor_t() {
//			printf("%d / %d\n", corr, all);
//			printf("%.8lf\n", corr * 1.0 / all);
			delete[] arr;
		}
	
};

#endif
